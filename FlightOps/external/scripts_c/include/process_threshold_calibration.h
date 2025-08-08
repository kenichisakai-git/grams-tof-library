#pragma once
#include <string>

bool runProcessThresholdCalibration(const std::string& configFile,
                                    const std::string& inputFilePrefix,
                                    const std::string& outFileName,
                                    const std::string& rootFileName = "");
