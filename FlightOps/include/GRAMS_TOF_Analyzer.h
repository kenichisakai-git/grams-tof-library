#pragma once
#include <string>
#include "FileType.h"

class GRAMS_TOF_Analyzer {
public:
    bool runPetsysProcessThresholdCalibration(const std::string& configFile,
                                              const std::string& inputFilePrefix,
                                              const std::string& outFileName,
                                              const std::string& rootFileName = "");

    bool runPetsysProcessTdcCalibration(const std::string& configFileName,
                                        const std::string& inputFilePrefix,
                                        const std::string& outputFilePrefix,
                                        const std::string& tmpFilePrefix = "",
                                        bool doSorting = true,
                                        bool keepTemporary = false,
                                        float nominalM = 200.0f);
    
    bool runPetsysProcessQdcCalibration(const std::string& configFileName,
                                        const std::string& inputFilePrefix,
                                        const std::string& outputFilePrefix,
                                        const std::string& tmpFilePrefix = "",
                                        bool doSorting = true,
                                        bool keepTemporary = false,
                                        float nominalM = 200.0f);
    
    bool runPetsysConvertRawToSingles(const std::string& configFileName,
                                      const std::string& inputFilePrefix,
                                      const std::string& outputFileName,
                                      PETSYS::FILE_TYPE fileType = PETSYS::FILE_TEXT,
                                      long long eventFractionToWrite = 1024,
                                      double fileSplitTime = 0.0);
};

