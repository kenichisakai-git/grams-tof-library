#pragma once
#include "GRAMS_TOF_Logger.h"
#include "FileType.h"
#include <string>
#include <functional>

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

    bool runPetsysConvertRawToRaw(const std::string& configFileName,
                                  const std::string& inputFilePrefix,
                                  const std::string& outputFileName,
                                  long long eventFractionToWrite = 1024);

    bool runPetsysConvertRawToSingles(const std::string& configFileName,
                                      const std::string& inputFilePrefix,
                                      const std::string& outputFileName,
                                      PETSYS::FILE_TYPE fileType = PETSYS::FILE_TEXT,
                                      long long eventFractionToWrite = 1024,
                                      double fileSplitTime = 0.0);

private:
    template<typename Func, typename... Args>
    bool safeRun(const std::string& name, Func&& func, Args&&... args) {
        try {
            return func(std::forward<Args>(args)...);
        }
        catch (const std::exception& e) {
            Logger::instance().error("[Analyzer] Exception in {}: {}", name, e.what());
            return false;
        }
        catch (...) {
            Logger::instance().error("[Analyzer] Unknown exception in {}", name);
            return false;
        }
    }
};

