#include "process_threshold_calibration.h"
#include <TApplication.h>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    TApplication app("app", &argc, argv);

    if (argc < 4) {
        std::cerr << "Usage: ./program --config CONFIG -i INPUT_PREFIX -o OUTPUT_FILE [--root-file ROOT_FILE]" << std::endl;
        return 1;
    }

    std::string configFile, inputFilePrefix, outFileName, rootFileName;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--config") configFile = argv[++i];
        else if (arg == "-i") inputFilePrefix = argv[++i];
        else if (arg == "-o") outFileName = argv[++i];
        else if (arg == "--root-file") rootFileName = argv[++i];
    }

    if (!runProcessThresholdCalibration(configFile, inputFilePrefix, outFileName, rootFileName)) {
        std::cerr << "Threshold calibration failed.\n";
        return 1;
    }

    return 0;
}

