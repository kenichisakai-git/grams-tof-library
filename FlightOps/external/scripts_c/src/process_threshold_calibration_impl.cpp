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

static TF1* createSigmoid() {
    TF1* fSigmoid = new TF1("fSigmoid", "[0]*ROOT::Math::normal_cdf(x,[1],[2])", 0, 64);
    fSigmoid->SetParName(0, "C");
    fSigmoid->SetParName(1, "#sigma");
    fSigmoid->SetParName(2, "x0");
    fSigmoid->SetNpx(64);
    return fSigmoid;
}

static vector<string> normalizeAndSplit(string line) {
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

static pair<double, double> getBaselineAndNoise(TProfile* profile, TF1* fSigmoid) {
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

static int getThresholdForRate(TProfile* profile, double rate) {
    int threshold = profile->FindFirstBinAbove(rate) - profile->FindBin(0);
    if (threshold > 63) return 63;
    else if (threshold < 0) return 63;
    else return threshold;
}

static double getDarkWidth(TProfile* profile, double baseline) {
    int maxBin = profile->GetMaximumBin();
    while (profile->GetBinCenter(maxBin) > baseline) {
        --maxBin;
        if (maxBin <= 0) break;  // safety check
    }

    double rate1pe = profile->GetBinContent(maxBin);
    int t1Bin = profile->FindFirstBinAbove(0.5 * rate1pe);
    double t1 = profile->GetBinCenter(t1Bin);

    return baseline - t1;
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

    auto safe_stoi = [](const std::string& s, int lineNo, const std::string& file) -> int {
        if (s.empty()) throw runtime_error("stoi: empty string at line " + to_string(lineNo) + " in " + file);
        try { return stoi(s); }
        catch (...) { throw runtime_error("stoi: invalid integer '" + s + "' at line " + to_string(lineNo) + " in " + file); }
    };

    auto safe_stod = [](const std::string& s, int lineNo, const std::string& file) -> double {
        if (s.empty()) throw runtime_error("stod: empty string at line " + to_string(lineNo) + " in " + file);
        try { return stod(s); }
        catch (...) { throw runtime_error("stod: invalid double '" + s + "' at line " + to_string(lineNo) + " in " + file); }
    };

    cout << "Load/Process baseline" << endl;

    map<ChannelKey, pair<int, int>> baselineSettings;
    set<ChannelKey> activeChannels;
    set<ChipKey> activeChips;

    ifstream infile(inputFilePrefix + "_baseline.tsv");
    string line;
    int lineNo = 0;
    while (getline(infile, line)) {
        lineNo++;
        vector<string> tokens = normalizeAndSplit(line);
        if (tokens.empty()) continue;
        int portID     = safe_stoi(tokens[0], lineNo, inputFilePrefix + "_baseline.tsv");
        int slaveID    = safe_stoi(tokens[1], lineNo, inputFilePrefix + "_baseline.tsv");
        int chipID     = safe_stoi(tokens[2], lineNo, inputFilePrefix + "_baseline.tsv");
        int channelID  = safe_stoi(tokens[3], lineNo, inputFilePrefix + "_baseline.tsv");
        int baseline_T = safe_stoi(tokens[4], lineNo, inputFilePrefix + "_baseline.tsv");
        int baseline_E = safe_stoi(tokens[5], lineNo, inputFilePrefix + "_baseline.tsv");
        ChannelKey ck = {portID, slaveID, chipID, channelID};
        ChipKey chip = {portID, slaveID, chipID};
        baselineSettings[ck] = {baseline_T, baseline_E};
        activeChannels.insert(ck);
        activeChips.insert(chip);
    }
   infile.close();

    cout << "Load/Process noise" << endl;
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
    lineNo = 0;
    while (getline(infile, line)) {
        lineNo++;
        vector<string> tokens = normalizeAndSplit(line);
        if (tokens.empty()) continue;
        if (tokens.size() < 7) {
            cerr << "Skipping malformed noise line " << lineNo << endl;
            continue;
        }
        int portID       = safe_stoi(tokens[0], lineNo, inputFilePrefix + "_noise.tsv");
        int slaveID      = safe_stoi(tokens[1], lineNo, inputFilePrefix + "_noise.tsv");
        int chipID       = safe_stoi(tokens[2], lineNo, inputFilePrefix + "_noise.tsv");
        int channelID    = safe_stoi(tokens[3], lineNo, inputFilePrefix + "_noise.tsv");
        string threshold = tokens[4];
        int thresholdValue   = safe_stoi(tokens[5], lineNo, inputFilePrefix + "_noise.tsv");
        double v            = safe_stod(tokens[6], lineNo, inputFilePrefix + "_noise.tsv") + ((rand() / double(RAND_MAX)) - 0.5) * 1E-6;
        noiseProfiles[{portID, slaveID, chipID, channelID, threshold}]->Fill(thresholdValue, v);
    }
    infile.close();

    cout << "Load/Process dark" << endl;
    map<tuple<int,int,int,int,string>, TProfile*> darkProfiles;
    for (auto& ck : activeChannels) {
        int portID, slaveID, chipID, channelID;
        tie(portID, slaveID, chipID, channelID) = ck;
        for (string threshold : {"vth_t1", "vth_t2", "vth_e"}) {
            string hName  = Form("hDark_%02d_%02d_%02d_%02d_%s", portID, slaveID, chipID, channelID, threshold.c_str());
            string hTitle = Form("Dark (%02d %02d %02d %02d) %s", portID, slaveID, chipID, channelID, threshold.c_str());
            darkProfiles[{portID, slaveID, chipID, channelID, threshold}] = new TProfile(hName.c_str(), hTitle.c_str(), 64, 0, 64);
        }
    }
    
    infile.open(inputFilePrefix + "_dark.tsv");
    lineNo = 0;
    while (getline(infile, line)) {
        lineNo++;
        vector<string> tokens = normalizeAndSplit(line);
        if (tokens.empty()) continue;
    
        int portID    = safe_stoi(tokens[0], lineNo, inputFilePrefix + "_dark.tsv");
        int slaveID   = safe_stoi(tokens[1], lineNo, inputFilePrefix + "_dark.tsv");
        int chipID    = safe_stoi(tokens[2], lineNo, inputFilePrefix + "_dark.tsv");
        int channelID = safe_stoi(tokens[3], lineNo, inputFilePrefix + "_dark.tsv");
        string threshold = tokens[4];
        int thresholdValue = safe_stoi(tokens[5], lineNo, inputFilePrefix + "_dark.tsv");
        double v = safe_stod(tokens[6], lineNo, inputFilePrefix + "_dark.tsv");
    
        darkProfiles[{portID, slaveID, chipID, channelID, threshold}]->Fill(thresholdValue, v);
    }
    infile.close();

    map<ChannelKey, pair<double,double>> thresholds;
    map<ChannelKey, double> darkRate;
    map<string, TGraph*> grBaseline, grZero, grNoise;

    ofstream outfile(outFileName);
    outfile << "# portID\tslaveID\tchipID\tchannelID\t";
    outfile << "baseline_T\tbaseline_E\t";
    outfile << "zero_T1\tzero_T2\tzero_E\t";
    outfile << "noise_T1\tnoise_T2\tnoise_E\n";

    for (auto& ck : activeChannels) {
        int portID, slaveID, chipID, channelID;
        tie(portID, slaveID, chipID, channelID) = ck;
    
        // Baseline
        int baseline_T = baselineSettings[ck].first;
        int baseline_E = baselineSettings[ck].second;
    
        // Zero and noise for each threshold
        double zero_T1, zero_T2, zero_E;
        double noise_T1, noise_T2, noise_E;
    
        tie(zero_T1, noise_T1) = getBaselineAndNoise(noiseProfiles[{portID, slaveID, chipID, channelID, "vth_t1"}], fSigmoid);
        tie(zero_T2, noise_T2) = getBaselineAndNoise(noiseProfiles[{portID, slaveID, chipID, channelID, "vth_t2"}], fSigmoid);
        tie(zero_E, noise_E)   = getBaselineAndNoise(noiseProfiles[{portID, slaveID, chipID, channelID, "vth_e"}], fSigmoid);
    
        // Store thresholds for possible later use
        thresholds[ck] = {zero_T1, zero_T2};
    
        // Prepare TGraphs for plotting
        string key = Form("%02d_%02d_%02d", portID, slaveID, chipID);
        if (!grBaseline.count(key)) grBaseline[key] = new TGraph();
        if (!grZero.count(key))     grZero[key] = new TGraph();
        if (!grNoise.count(key))    grNoise[key] = new TGraph();
    
        int index = channelID;
        grBaseline[key]->SetPoint(index, channelID, baseline_E);
        grZero[key]->SetPoint(index, channelID, zero_T1); // Python uses zero_T1 for T1
        grNoise[key]->SetPoint(index, channelID, noise_T1); // Python noise_T1
    
        // Write output exactly like Python
        outfile << portID << '\t' << slaveID << '\t' << chipID << '\t' << channelID << '\t';
        outfile << baseline_T << '\t' << baseline_E << '\t';
        outfile << zero_T1 << '\t' << zero_T2 << '\t' << zero_E << '\t';
        outfile << noise_T1 << '\t' << noise_T2 << '\t' << noise_E << '\n';
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

        canvas->SaveAs(Form("%s_%02d_%02d_%02d.svg", inputFilePrefix.c_str(), portID, slaveID, chipID));
    }

    if (rootFile) {
        rootFile->Write();
        rootFile->Close();
    }

    return true;
}

