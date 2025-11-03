#pragma once
#include <string>
#include <SystemConfig.h>

bool runConvertRawToRaw(const std::string& configFileName,
                        const std::string& inputFilePrefix,
                        const std::string& outputFileName,
                        long long eventFractionToWrite = 1024);

