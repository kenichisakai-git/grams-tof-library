// GRAMS_TOF_PythonIntegration.h
#pragma once

#include <memory>
#include <string>
#include <vector>

class GRAMS_TOF_DAQManager;

class GRAMS_TOF_PythonIntegration {
public:
    explicit GRAMS_TOF_PythonIntegration(GRAMS_TOF_DAQManager& daq);
    ~GRAMS_TOF_PythonIntegration();

    void loadPythonScript(const std::string& filename);
    void execPythonFunction(const std::string& func_name);
    bool runMakeBiasCalibrationTable(const std::string& scriptModule,
                                     const std::string& outputFile,
                                     const std::vector<int>& portIDs,
                                     const std::vector<int>& slaveIDs,
                                     const std::vector<int>& slotIDs,
                                     const std::vector<std::string>& filenames);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

