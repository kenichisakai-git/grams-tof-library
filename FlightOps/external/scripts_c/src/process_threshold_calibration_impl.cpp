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

// Helper to create the sigmoid function for fitting noise profiles
static TF1* createSigmoid() {
    TF1* fSigmoid = new TF1("fSigmoid", "[0]*ROOT::Math::normal_cdf(x,[1],[2])", 0, 64);
    fSigmoid->SetParName(0, "C");
    fSigmoid->SetParName(1, "#sigma");
    fSigmoid->SetParName(2, "x0");
    fSigmoid->SetNpx(64);
    return fSigmoid;
}

// Helper to sanitize and split TSV lines
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

// Fit the sigmoid to extract the zero-point and noise width
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

bool runProcessThresholdCalibration(const std::string& configFile,
                                    const std::string& inputFilePrefix,
                                    const std::string& outFileName,
                                    const std::string& rootFileName) {

    if (configFile.empty() || inputFilePrefix.empty() || outFileName.empty()) {
        cerr << "Error: config, input, and output arguments are mandatory." << endl;
        return false;
    }

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

    // --- 1. Load Baseline Data ---
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

    // --- 2. Load Noise Data ---
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
        if (tokens.size() < 7) continue;

        int portID       = safe_stoi(tokens[0], lineNo, inputFilePrefix + "_noise.tsv");
        int slaveID      = safe_stoi(tokens[1], lineNo, inputFilePrefix + "_noise.tsv");
        int chipID       = safe_stoi(tokens[2], lineNo, inputFilePrefix + "_noise.tsv");
        int channelID    = safe_stoi(tokens[3], lineNo, inputFilePrefix + "_noise.tsv");
        string threshold = tokens[4];
        int thresholdValue = safe_stoi(tokens[5], lineNo, inputFilePrefix + "_noise.tsv");
        double v           = safe_stod(tokens[6], lineNo, inputFilePrefix + "_noise.tsv") + ((rand() / double(RAND_MAX)) - 0.5) * 1E-6;
        
        noiseProfiles[{portID, slaveID, chipID, channelID, threshold}]->Fill(thresholdValue, v);
    }
    infile.close();

    // --- 3. Process and Output Results ---
    map<string, TGraph*> grBaseline, grZero, grNoise;
    ofstream outfile(outFileName);
    outfile << "# portID\tslaveID\tchipID\tchannelID\tbaseline_T\tbaseline_E\tzero_T1\tzero_T2\tzero_E\tnoise_T1\tnoise_T2\tnoise_E\n";

    for (auto& ck : activeChannels) {
        int portID, slaveID, chipID, channelID;
        tie(portID, slaveID, chipID, channelID) = ck;
    
        double zero_T1, zero_T2, zero_E;
        double noise_T1, noise_T2, noise_E;
    
        // Calculate noise and zero-points
        tie(zero_T1, noise_T1) = getBaselineAndNoise(noiseProfiles[{portID, slaveID, chipID, channelID, "vth_t1"}], fSigmoid);
        tie(zero_T2, noise_T2) = getBaselineAndNoise(noiseProfiles[{portID, slaveID, chipID, channelID, "vth_t2"}], fSigmoid);
        tie(zero_E, noise_E)   = getBaselineAndNoise(noiseProfiles[{portID, slaveID, chipID, channelID, "vth_e"}], fSigmoid);
    
        // Prepare data for plotting
        string key = Form("%02d_%02d_%02d", portID, slaveID, chipID);
        if (!grBaseline.count(key)) grBaseline[key] = new TGraph();
        if (!grZero.count(key))     grZero[key] = new TGraph();
        if (!grNoise.count(key))    grNoise[key] = new TGraph();
    
        grBaseline[key]->SetPoint(channelID, channelID, (double)baselineSettings[ck].second);
        grZero[key]->SetPoint(channelID, channelID, zero_T1);
        grNoise[key]->SetPoint(channelID, channelID, noise_T1);
    
        // Write the consolidated output
        outfile << portID << '\t' << slaveID << '\t' << chipID << '\t' << channelID << '\t'
                << baselineSettings[ck].first << '\t' << baselineSettings[ck].second << '\t'
                << zero_T1 << '\t' << zero_T2 << '\t' << zero_E << '\t'
                << noise_T1 << '\t' << noise_T2 << '\t' << noise_E << '\n';
    }
    outfile.close();

    // --- 4. Plotting (SVG Generation) ---
    for (auto& chip : activeChips) {
        int portID, slaveID, chipID;
        tie(portID, slaveID, chipID) = chip;
        string key = Form("%02d_%02d_%02d", portID, slaveID, chipID);

        auto canvas = new TCanvas(Form("c_%s", key.c_str()), key.c_str(), 800, 600);
        auto mg = new TMultiGraph();
        
        grBaseline[key]->SetMarkerStyle(20); grBaseline[key]->SetMarkerColor(kRed); grBaseline[key]->SetTitle("vth_e");
        grZero[key]->SetMarkerStyle(21);     grZero[key]->SetMarkerColor(kBlue);    grZero[key]->SetTitle("vth_t1");
        grNoise[key]->SetMarkerStyle(22);    grNoise[key]->SetMarkerColor(kGreen+2); grNoise[key]->SetTitle("noise");

        mg->Add(grBaseline[key], "P");
        mg->Add(grZero[key], "P");
        mg->Add(grNoise[key], "P");
        mg->Draw("A PMC");
        mg->SetTitle(Form("Threshold Summary %s;Channel ID;Threshold", key.c_str()));

        auto legend = new TLegend(0.7, 0.7, 0.9, 0.9);
        legend->AddEntry(grBaseline[key], "vth_e", "P");
        legend->AddEntry(grZero[key], "vth_t1", "P");
        legend->AddEntry(grNoise[key], "3#sigma", "P");
        legend->Draw();

        canvas->SaveAs(Form("%s_%02d_%02d_%02d.svg", inputFilePrefix.c_str(), portID, slaveID, chipID));
        delete canvas;
    }

    if (rootFile) {
        rootFile->Write();
        rootFile->Close();
    }

    return true;
}
