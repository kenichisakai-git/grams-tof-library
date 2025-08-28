#include "process_threshold_calibration.h"
#include <TApplication.h>
#include <CLI11.hpp>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    CLI::App cli{"Process threshold calibration"};

    std::string configFile, inputFilePrefix, outFileName, rootFileName;
    cli.add_option("--config", configFile, "Configuration file")->required();
    cli.add_option("-i", inputFilePrefix, "Input prefix")->required();
    cli.add_option("-o", outFileName, "Output file")->required();
    cli.add_option("--root-file", rootFileName, "Optional ROOT file")->default_val("");

    CLI11_PARSE(cli, argc, argv);

    // Create TApplication only after parsing CLI11
    int fake_argc = 1;
    char* fake_argv[] = {argv[0], nullptr};
    TApplication app("app", &fake_argc, fake_argv);

    if (!runProcessThresholdCalibration(configFile, inputFilePrefix, outFileName, rootFileName)) {
        std::cerr << "Threshold calibration failed.\n";
        return 1;
    }

    return 0;
}

