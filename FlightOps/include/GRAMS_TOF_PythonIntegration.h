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

    template <typename... Args>
    bool runPythonFunction(const std::string& scriptPath,
                           const std::string& functionName,
                           Args&&... args);
    bool runPetsysInitSystem(const std::string& scriptModule);
    bool runPetsysMakeBiasCalibrationTable(const std::string& scriptModule,
                                           const std::string& outputFile,
                                           const std::vector<int>& portIDs,
                                           const std::vector<int>& slaveIDs,
                                           const std::vector<int>& slotIDs,
                                           const std::vector<std::string>& filenames);
    bool runPetsysMakeSimpleBiasSettingsTable(const std::string& scriptPath,
                                              const std::string& configPath,
                                              float offset,
                                              float prebd,
                                              float bd,
                                              float over,
                                              const std::string& outputPath);
    bool runPetsysMakeSimpleChannelMap(const std::string& scriptPath,
                                       const std::string& outputFile);
    bool runPetsysMakeSimpleDiscSettingsTable(const std::string& scriptPath,
                                              const std::string& configPath,
                                              int vth_t1,
                                              int vth_t2,
                                              int vth_e,
                                              const std::string& outputPath);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

