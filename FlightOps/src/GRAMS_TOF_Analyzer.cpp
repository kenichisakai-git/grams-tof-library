#include "GRAMS_TOF_Analyzer.h"
#include "process_threshold_calibration.h" 

bool GRAMS_TOF_Analyzer::runPetsysProcessThresholdCalibration(
    const std::string& configFile,
    const std::string& inputFilePrefix,
    const std::string& outFileName,
    const std::string& rootFileName) {
    return runProcessThresholdCalibration(configFile, inputFilePrefix, outFileName, rootFileName);
}

