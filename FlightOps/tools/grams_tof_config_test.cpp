#include "GRAMS_TOF_Config.h"
#include <iostream>

int main() {
    GRAMS_TOF_Config config;
    if (!config.load("config/config.ini")) {
        std::cerr << "Failed to load config.ini\n";
        return 1;
    }

    try {
        std::string biasFile = config.getString("main", "bias_settings_table");
        int threshold = config.getInt("hw_trigger", "threshold");
        double maxEnergy = config.getDouble("sw_trigger", "group_max_energy");

        std::cout << "Bias file: " << biasFile << "\n";
        std::cout << "HW Threshold: " << threshold << "\n";
        std::cout << "Max Energy: " << maxEnergy << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Config error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

