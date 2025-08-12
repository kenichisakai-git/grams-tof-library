#pragma once

#include <unordered_map>
#include <functional>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

#include "GRAMS_TOF_DAQManager.h"
#include "GRAMS_TOF_CommandCodec.h"
#include "GRAMS_TOF_PythonIntegration.h"
#include "GRAMS_TOF_Analyzer.h"

class GRAMS_TOF_CommandDispatch {
public:
    GRAMS_TOF_CommandDispatch(GRAMS_TOF_PythonIntegration& pyint, GRAMS_TOF_Analyzer& analyzer);
    ~GRAMS_TOF_CommandDispatch();

    bool dispatch(TOFCommandCode code, const std::vector<int>& argv);

private:
    GRAMS_TOF_PythonIntegration& pyint_;
    GRAMS_TOF_Analyzer& analyzer_;
    std::unordered_map<TOFCommandCode, std::function<bool(const std::vector<int>&)>> table_;

    // Thread and control for DAQ run
    std::thread daqThread_;
    std::atomic<bool> daqRunning_{false};
    std::mutex daqMutex_;

    // Thread function to run DAQ
    void runDAQThread();
};

