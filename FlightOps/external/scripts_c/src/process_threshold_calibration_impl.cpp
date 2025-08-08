#include "process_threshold_calibration.h"

#include <TFile.h>
#include <TProfile.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TLegend.h>
#include <TMultiGraph.h>
#include <TApplication.h>
#include <TSystem.h>
#include <TAxis.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <tuple>
#include <algorithm>
#include <regex>
#include <cmath>
#include <cstdlib>


using namespace std;

using ChannelKey = tuple<int, int, int, int>;
using ChipKey = tuple<int, int, int>;

TF1* createSigmoid() {
    TF1* fSigmoid = new TF1("fSigmoid", "[0]*ROOT::Math::normal_cdf(x,[1],[2])", 0, 64);
    fSigmoid->SetParName(0, "C");
    fSigmoid->SetParName(1, "#sigma");
    fSigmoid->SetParName(2, "x0");
    fSigmoid->SetNpx(64);
    return fSigmoid;
}

vector<string> normalizeAndSplit(string line) {
    line = regex_replace(line, regex("\\s*#.*"), "");
    line = regex_replace(line, regex("^\\s*"), "");
    line = regex_replace(line, regex("\\s*$"), "");
    line = regex_replace(line, regex("\\s+"), "\t");
    line = regex_replace(line, regex("\\r"), "");
    line = regex_replace(line, regex("\\n"), "");

    vector<string> result;
    stringstream ss(line);
    string item;
    while (getline(ss, item, '\t')) {
        result.push_back(item);
    }
    return result;
}

pair<double, double> getBaselineAndNoise(TProfile* profile, TF1* fSigmoid) {
    int lowBin = profile->FindFirstBinAbove(0.1);
    int highBin = profile->FindFirstBinAbove(0.9);
    double low = profile->GetBinCenter(lowBin) - 0.5;
    double high = profile->GetBinCenter(highBin) + 0.5;

    fSigmoid->FixParameter(0, 1.0);
    fSigmoid->SetParameter(1, high - low);
    fSigmoid->SetParLimits(1, 0.05, high - low);
    fSigmoid->SetParameter(2, (low + high) / 2.0);
    fSigmoid->SetParLimits(2, low, high);

    profile->Fit(fSigmoid, "Q", "", 0, 64);
    TF1* f = profile->GetFunction("fSigmoid");

    if (f) {
        return {f->GetParameter(2), f->GetParameter(1)};
    } else {
        cerr << "WARNING: No fit for " << profile->GetName() << endl;
        return {(low + high) / 2.0, 0.05};
    }
}


bool runProcessThresholdCalibration(const std::string& configFile,
                                    const std::string& inputFilePrefix,
                                    const std::string& outFileName,
                                    const std::string& rootFileName) {

    if (configFile.empty() || inputFilePrefix.empty() || outFileName.empty()) {
        cerr << "Error: config, input, and output arguments are mandatory." << endl;
        return false;
    }

    cout << "Running process_threshold_calibration with parameters:" << endl;
    cout << "  Config file: " << configFile << endl;
    cout << "  Input prefix: " << inputFilePrefix << endl;
    cout << "  Output file: " << outFileName << endl;
    if (!rootFileName.empty())
        cout << "  Root file: " << rootFileName << endl;

    TFile* rootFile = nullptr;
    if (!rootFileName.empty()) rootFile = new TFile(rootFileName.c_str(), "RECREATE");

    TF1* fSigmoid = createSigmoid();

    map<ChannelKey, pair<int, int>> baselineSettings;
    set<ChannelKey> activeChannels;
    set<ChipKey> activeChips;

    ifstream infile(inputFilePrefix + "_baseline.tsv");
    string line;
    while (getline(infile, line)) {
        vector<string> tokens = normalizeAndSplit(line);
        if (tokens.empty()) continue;
        int portID = stoi(tokens[0]);
        int slaveID = stoi(tokens[1]);
        int chipID = stoi(tokens[2]);
        int channelID = stoi(tokens[3]);
        int baseline_T = stoi(tokens[4]);
        int baseline_E = stoi(tokens[5]);
        ChannelKey ck = {portID, slaveID, chipID, channelID};
        ChipKey chip = {portID, slaveID, chipID};
        baselineSettings[ck] = {baseline_T, baseline_E};
        activeChannels.insert(ck);
        activeChips.insert(chip);
    }
    infile.close();

    map<tuple<int, int, int, int, string>, TProfile*> noiseProfiles;
    for (auto& ck : activeChannels) {
        int portID, slaveID, chipID, channelID;
        tie(portID, slaveID, chipID, channelID) = ck;
        for (string threshold : {"vth_t1", "vth_t2", "vth_e"}) {
            string hName = Form("hNoise_%02d_%02d_%02d_%02d_%s", portID, slaveID, chipID, channelID, threshold.c_str());
            string hTitle = Form("Noise (%02d %02d %02d %02d) %s", portID, slaveID, chipID, channelID, threshold.c_str());
            noiseProfiles[{portID, slaveID, chipID, channelID, threshold}] = new TProfile(hName.c_str(), hTitle.c_str(), 64, 0, 64);
        }
    }

    infile.open(inputFilePrefix + "_noise.tsv");
    while (getline(infile, line)) {
        vector<string> tokens = normalizeAndSplit(line);
        if (tokens.empty()) continue;
        int portID = stoi(tokens[0]);
        int slaveID = stoi(tokens[1]);
        int chipID = stoi(tokens[2]);
        int channelID = stoi(tokens[3]);
        string thresholdName = tokens[4];
        int thresholdValue = stoi(tokens[5]);
        double v = stod(tokens[6]) + ((rand() / double(RAND_MAX)) - 0.5) * 1E-6;
        noiseProfiles[{portID, slaveID, chipID, channelID, thresholdName}]->Fill(thresholdValue, v);
    }
    infile.close();

    map<ChannelKey, pair<double, double>> thresholds;
    map<ChannelKey, double> darkRate;
    map<string, TGraph*> grBaseline, grZero, grNoise;

    infile.open(inputFilePrefix + "_dark.tsv");
    while (getline(infile, line)) {
        vector<string> tokens = normalizeAndSplit(line);
        if (tokens.empty()) continue;
        int portID = stoi(tokens[0]);
        int slaveID = stoi(tokens[1]);
        int chipID = stoi(tokens[2]);
        int channelID = stoi(tokens[3]);
        double rate = stod(tokens[4]);
        ChannelKey ck = {portID, slaveID, chipID, channelID};
        darkRate[ck] = rate;
    }
    infile.close();

    ofstream outfile(outFileName);
    outfile << "portID\tslaveID\tchipID\tchannelID\tvth_t1\tvth_t2\tvth_e\tsigma\tdark" << endl;

    for (auto& ck : activeChannels) {
        int portID, slaveID, chipID, channelID;
        tie(portID, slaveID, chipID, channelID) = ck;

        auto p1 = noiseProfiles[{portID, slaveID, chipID, channelID, "vth_t1"}];
        auto p2 = noiseProfiles[{portID, slaveID, chipID, channelID, "vth_t2"}];
        auto pe = noiseProfiles[{portID, slaveID, chipID, channelID, "vth_e"}];

        auto result = getBaselineAndNoise(p1, fSigmoid);
        double vth_t1 = result.first;
        double sigma = result.second;

        result = getBaselineAndNoise(p2, fSigmoid);
        double vth_t2 = result.first;

        result = getBaselineAndNoise(pe, fSigmoid);
        double vth_e = result.first;

        double noise = sigma * 3.0;
        double dark = darkRate.count(ck) ? darkRate[ck] : 0;

        thresholds[ck] = {vth_t1, vth_t2};

        string key = Form("%02d_%02d_%02d", portID, slaveID, chipID);
        if (!grBaseline.count(key)) grBaseline[key] = new TGraph();
        if (!grZero.count(key)) grZero[key] = new TGraph();
        if (!grNoise.count(key)) grNoise[key] = new TGraph();

        int index = channelID;
        grBaseline[key]->SetPoint(index, channelID, vth_e);
        grZero[key]->SetPoint(index, channelID, vth_t1);
        grNoise[key]->SetPoint(index, channelID, noise);

        outfile << portID << '\t' << slaveID << '\t' << chipID << '\t' << channelID << '\t';
        outfile << int(vth_t1 + 0.5) << '\t' << int(vth_t2 + 0.5) << '\t' << int(vth_e + 0.5) << '\t';
        outfile << sigma << '\t' << dark << endl;
    }
    outfile.close();

    for (auto& chip : activeChips) {
        int portID, slaveID, chipID;
        tie(portID, slaveID, chipID) = chip;
        string key = Form("%02d_%02d_%02d", portID, slaveID, chipID);

        auto canvas = new TCanvas(Form("c_%s", key.c_str()), key.c_str(), 800, 600);
        auto mg = new TMultiGraph();
        grBaseline[key]->SetMarkerStyle(20);
        grBaseline[key]->SetMarkerColor(kRed);
        grBaseline[key]->SetTitle("vth_e");
        grZero[key]->SetMarkerStyle(21);
        grZero[key]->SetMarkerColor(kBlue);
        grZero[key]->SetTitle("vth_t1");
        grNoise[key]->SetMarkerStyle(22);
        grNoise[key]->SetMarkerColor(kGreen+2);
        grNoise[key]->SetTitle("noise");

        mg->Add(grBaseline[key], "P");
        mg->Add(grZero[key], "P");
        mg->Add(grNoise[key], "P");

        mg->Draw("A PMC");
        mg->SetTitle(Form("Threshold Summary %s;Channel ID;Threshold", key.c_str()));
        mg->GetXaxis()->SetLimits(0, 64);

        auto legend = new TLegend(0.7, 0.7, 0.9, 0.9);
        legend->AddEntry(grBaseline[key], "vth_e", "P");
        legend->AddEntry(grZero[key], "vth_t1", "P");
        legend->AddEntry(grNoise[key], "3#sigma", "P");
        legend->Draw();

        canvas->Write();
    }

    if (rootFile) {
        rootFile->Write();
        rootFile->Close();
    }

    return 0;
}

