#include "process_tdc_calibration.h"
#include <iostream>
#include <string>
#include <getopt.h>

int main(int argc, char** argv) {
    float nominalM = 200.0f;
    std::string configFileName;
    std::string inputFilePrefix;
    std::string outputFilePrefix;
    std::string tmpFilePrefix;
    bool doSorting = true;
    bool keepTemporary = false;

    static struct option longOptions[] = {
        { "help",           no_argument,       0, 0 },
        { "config",         required_argument, 0, 0 },
        { "no-sorting",     no_argument,       0, 0 },
        { "keep-temporary", no_argument,       0, 0 },
        { "tmp-prefix",     required_argument, 0, 0 }
    };

    while (true) {
        int optionIndex = 0;
        int c = getopt_long(argc, argv, "i:o:c:", longOptions, &optionIndex);

        if (c == -1) break;
        else if (c != 0) {
            switch (c) {
                case 'i': inputFilePrefix = optarg; break;
                case 'o': outputFilePrefix = optarg; break;
                case 'c': configFileName = optarg; break;
                default:
                    std::cerr << "Invalid short option\n";
                    return 1;
            }
        }
        else if (c == 0) {
            switch (optionIndex) {
                case 0:
                    std::cout << "Usage: ./process_tdc_calibration --config FILE -i INPUT -o OUTPUT [options]\n";
                    return 0;
                case 1: configFileName = optarg; break;
                case 2: doSorting = false; break;
                case 3: keepTemporary = true; break;
                case 4: tmpFilePrefix = optarg; break;
                default: return 1;
            }
        }
    }

    if (tmpFilePrefix.empty())
        tmpFilePrefix = outputFilePrefix;

    if (!runProcessTdcCalibration(configFileName, inputFilePrefix, outputFilePrefix, tmpFilePrefix, doSorting, keepTemporary, nominalM)) {
        std::cerr << "TDC calibration failed.\n";
        return 1;
    }

    return 0;
}

