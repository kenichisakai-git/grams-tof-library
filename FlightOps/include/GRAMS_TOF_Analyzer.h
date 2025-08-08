#pragma once
#include <string>

class GRAMS_TOF_Analyzer {
public:
    bool runPetsysProcessThresholdCalibration(const std::string& configFile,
                                              const std::string& inputFilePrefix,
                                              const std::string& outFileName,
                                              const std::string& rootFileName = "");
};

