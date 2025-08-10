#pragma once
#include <string>
#include <SystemConfig.h>
#include "FileType.h"

bool runConvertRawToSingles(const std::string& configFileName,
                            const std::string& inputFilePrefix,
                            const std::string& outputFileName,
                            PETSYS::FILE_TYPE fileType = PETSYS::FILE_TEXT,
                            long long eventFractionToWrite = 1024,
                            double fileSplitTime = 0.0);

