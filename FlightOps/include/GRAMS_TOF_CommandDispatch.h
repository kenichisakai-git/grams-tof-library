#pragma once

#include <unordered_map>
#include <functional>
#include <string>
#include <vector>

#include "GRAMS_TOF_DAQManager.h"
#include "GRAMS_TOF_CommandCodec.h"
#include "GRAMS_TOF_PythonIntegration.h"

class GRAMS_TOF_CommandDispatch {
public:
    GRAMS_TOF_CommandDispatch(GRAMS_TOF_PythonIntegration& pyint);
    bool dispatch(TOFCommandCode code, const std::vector<int>& argv) const;

private:
    GRAMS_TOF_PythonIntegration& pyint_;
    std::unordered_map<TOFCommandCode, std::function<bool(const std::vector<int>&)>> table_;
};

