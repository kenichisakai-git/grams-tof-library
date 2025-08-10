#include "convert_raw_to_singles.h"
#include <iostream>
#include <string>
#include <getopt.h>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <SystemConfig.h>

using namespace PETSYS;

int main(int argc, char** argv) {
    std::string configFileName;
    std::string inputFilePrefix;
    std::string outputFileName;
    FILE_TYPE fileType = FILE_TEXT;
    long long eventFractionToWrite = 1024;
    double fileSplitTime = 0.0;

    static struct option longOptions[] = {
        { "help",           no_argument,       0, 0 },
        { "config",         required_argument, 0, 0 },
        { "writeBinary",    no_argument,       0, 0 },
        { "writeRoot",      no_argument,       0, 0 },
        { "writeFraction",  required_argument, 0, 0 },
        { "splitTime",      required_argument, 0, 0 }
    };

    while (true) {
        int optionIndex = 0;
        int c = getopt_long(argc, argv, "i:o:c:", longOptions, &optionIndex);

        if (c == -1) break;
        else if (c != 0) {
            switch (c) {
                case 'i': inputFilePrefix = optarg; break;
                case 'o': outputFileName = optarg; break;
                case 'c': configFileName = optarg; break;
                default:
                    std::cerr << "Invalid short option\n";
                    return 1;
            }
        }
        else if (c == 0) {
            switch (optionIndex) {
                case 0:
                    std::cout << "Usage: ./convert_raw_to_singles --config FILE -i INPUT -o OUTPUT [options]\n";
                    return 0;
                case 1: configFileName = optarg; break;
                case 2: fileType = FILE_BINARY; break;
                case 3: fileType = FILE_ROOT; break;
                case 4: eventFractionToWrite = std::llround(1024 * boost::lexical_cast<float>(optarg) / 100.0); break;
                case 5: fileSplitTime = boost::lexical_cast<double>(optarg); break;
                default: return 1;
            }
        }
    }

    if (!runConvertRawToSingles(configFileName, inputFilePrefix, outputFileName, fileType, eventFractionToWrite, fileSplitTime)) {
        std::cerr << "Conversion from raw to singles failed.\n";
        return 1;
    }

    return 0;
}

