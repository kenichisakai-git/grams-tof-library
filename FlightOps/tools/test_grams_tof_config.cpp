#include "GRAMS_TOF_Config.h"
#include <iostream>

int main() {
    try {
        //GRAMS_TOF_Config::instance().setConfigFile("config/config.ini");
    } catch (const std::exception& e) {
        std::cerr << "Config load error: " << e.what() << "\n";
        return 1;
    }

    try {
        auto& config = GRAMS_TOF_Config::instance();
        std::string biasFile = config.getString("main", "bias_settings_table");

        int threshold = config.getInt("hw_trigger", "threshold");
        double maxEnergy = config.getDouble("sw_trigger", "group_max_energy");

        std::cout << "Config file: " << config.getConfigFilePath() << std::endl; 
        std::cout << "Bias file: " << biasFile << std::endl;
        std::cout << "HW Threshold: " << threshold << std::endl;
        std::cout << "Max Energy: " << maxEnergy << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Config error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

