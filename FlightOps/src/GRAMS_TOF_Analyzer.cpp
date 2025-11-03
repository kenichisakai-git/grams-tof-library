#include "GRAMS_TOF_Analyzer.h"
#include "process_threshold_calibration.h"
#include "process_tdc_calibration.h"
#include "process_qdc_calibration.h"
#include "convert_raw_to_raw.h"
#include "convert_raw_to_singles.h"

bool GRAMS_TOF_Analyzer::runPetsysProcessThresholdCalibration(
    const std::string& configFile,
    const std::string& inputFilePrefix,
    const std::string& outFileName,
    const std::string& rootFileName)
{
    return safeRun("runPetsysProcessThresholdCalibration",
                   runProcessThresholdCalibration,
                   configFile, inputFilePrefix, outFileName, rootFileName);
}

bool GRAMS_TOF_Analyzer::runPetsysProcessTdcCalibration(
    const std::string& configFileName,
    const std::string& inputFilePrefix,
    const std::string& outputFilePrefix,
    const std::string& tmpFilePrefix,
    bool doSorting,
    bool keepTemporary,
    float nominalM)
{
    return safeRun("runPetsysProcessTdcCalibration",
                   runProcessTdcCalibration,
                   configFileName, inputFilePrefix, outputFilePrefix,
                   tmpFilePrefix, doSorting, keepTemporary, nominalM);
}

bool GRAMS_TOF_Analyzer::runPetsysProcessQdcCalibration(
    const std::string& configFileName,
    const std::string& inputFilePrefix,
    const std::string& outputFilePrefix,
    const std::string& tmpFilePrefix,
    bool doSorting,
    bool keepTemporary,
    float nominalM)
{
    return safeRun("runPetsysProcessQdcCalibration",
                   runProcessQdcCalibration,
                   configFileName, inputFilePrefix, outputFilePrefix,
                   tmpFilePrefix, doSorting, keepTemporary, nominalM);
}

bool GRAMS_TOF_Analyzer::runPetsysConvertRawToRaw(
    const std::string& configFileName,
    const std::string& inputFilePrefix,
    const std::string& outputFileName,
    long long eventFractionToWrite)
{
    return safeRun("runPetsysConvertRawToRaw",
                   runConvertRawToRaw,
                   configFileName, inputFilePrefix, outputFileName,
                   eventFractionToWrite);
}

bool GRAMS_TOF_Analyzer::runPetsysConvertRawToSingles(
    const std::string& configFileName,
    const std::string& inputFilePrefix,
    const std::string& outputFileName,
    PETSYS::FILE_TYPE fileType,
    long long eventFractionToWrite,
    double fileSplitTime)
{
    return safeRun("runPetsysConvertRawToSingles",
                   runConvertRawToSingles,
                   configFileName, inputFilePrefix, outputFileName,
                   fileType, eventFractionToWrite, fileSplitTime);
}

