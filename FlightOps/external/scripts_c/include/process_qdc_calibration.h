#pragma once
#include <string>

bool runProcessQdcCalibration(const std::string& configFileName,
                              const std::string& inputFilePrefix,
                              const std::string& outputFilePrefix,
                              const std::string& tmpFilePrefix = "",
                              bool doSorting = true,
                              bool keepTemporary = false,
                              float nominalM = 200.0f);

