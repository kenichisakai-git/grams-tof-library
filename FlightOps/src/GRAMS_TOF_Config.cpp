#include "GRAMS_TOF_Config.h"
#include "INIReader.h"  // Assumes inih is in your include path

#include <cstdlib>
#include <stdexcept>
#include <filesystem>

bool GRAMS_TOF_Config::load(const std::string& filename) {
    configDir_ = std::filesystem::path(filename).parent_path().string();

    INIReader reader(filename);
    if (reader.ParseError() < 0) {
        return false;
    }

    for (const auto& section : reader.Sections()) {
        for (const auto& key : reader.GetKeys(section)) {
            std::string rawValue = reader.Get(section, key, "");
            data_[section][key] = substituteVariables(rawValue);
        }
    }

    return true;
}

std::string GRAMS_TOF_Config::substituteVariables(const std::string& value) const {
    std::string result = value;

    size_t pos;
    if ((pos = result.find("%CDIR%")) != std::string::npos) {
        result.replace(pos, 6, configDir_);
    }
    if ((pos = result.find("%PWD%")) != std::string::npos) {
        result.replace(pos, 5, ".");
    }
    if ((pos = result.find("%HOME%")) != std::string::npos) {
        const char* home = std::getenv("HOME");
        if (home)
            result.replace(pos, 6, home);
    }

    return result;
}

std::string GRAMS_TOF_Config::getString(const std::string& section, const std::string& key) const {
    auto secIt = data_.find(section);
    if (secIt == data_.end()) throw std::runtime_error("Missing section: " + section);
    auto keyIt = secIt->second.find(key);
    if (keyIt == secIt->second.end()) throw std::runtime_error("Missing key: " + key);
    return keyIt->second;
}

int GRAMS_TOF_Config::getInt(const std::string& section, const std::string& key) const {
    return std::stoi(getString(section, key));
}

double GRAMS_TOF_Config::getDouble(const std::string& section, const std::string& key) const {
    return std::stod(getString(section, key));
}

