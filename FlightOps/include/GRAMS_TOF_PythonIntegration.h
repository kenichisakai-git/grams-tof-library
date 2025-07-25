// GRAMS_TOF_PythonIntegration.h
#pragma once

#include <memory>
#include <string>

class GRAMS_TOF_DAQManager;

class GRAMS_TOF_PythonIntegration {
public:
    explicit GRAMS_TOF_PythonIntegration(GRAMS_TOF_DAQManager& daq);
    ~GRAMS_TOF_PythonIntegration();

    void loadPythonScript(const std::string& filename);
    void execPythonFunction(const std::string& func_name);
    bool runUserScriptByCommand(const std::string& cmd);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

