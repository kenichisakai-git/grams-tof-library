#include "GRAMS_TOF_Config.h"
#include "INIReader.h"

#include <cstdlib>
#include <stdexcept>
#include <filesystem>
#include <iostream>

namespace {
    // Define the relative path to the config file from $GLIB
    const std::filesystem::path RELATIVE_CONFIG_PATH = std::filesystem::path("config") / "config.ini";
}

GRAMS_TOF_Config& GRAMS_TOF_Config::instance() {
    static GRAMS_TOF_Config instance;
    return instance;
}

bool GRAMS_TOF_Config::loadDefaultConfig() {
    GRAMS_TOF_Config& config = instance();
    if (config.loaded_) return true; // Already loaded, skip.

    const char* glibPath = std::getenv("GLIB");
    if (!glibPath) {
        std::cerr << "Warning: GLIB environment variable not set. Default config not loaded." << std::endl;
        return false;
    }

    std::filesystem::path defaultPath = std::filesystem::path(glibPath) / RELATIVE_CONFIG_PATH;
    std::string defaultPathStr = defaultPath.string();

    if (config.load(defaultPathStr)) {
        config.configFilePath_ = defaultPathStr;
        config.loaded_ = true;
        std::cout << "Info: Default config loaded from GLIB: " << defaultPathStr << std::endl;
        return true;
    } else {
        std::cerr << "Warning: Failed to load default config from " << defaultPathStr << std::endl;
        return false;
    }
}

void GRAMS_TOF_Config::setConfigFile(const std::string& filename) {
    if (loaded_) {
        data_.clear();
    }
    if (!load(filename)) {
        // The empty string path was failing here!
        throw std::runtime_error("Failed to load config file: " + filename);
    }
    configFilePath_ = filename;
    loaded_ = true;
}

bool GRAMS_TOF_Config::load(const std::string& filename) {
    configDir_ = std::filesystem::path(filename).parent_path().string();

    INIReader reader(filename);
    if (reader.ParseError() < 0) {
        return false;
    }

    data_.clear();
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

std::string GRAMS_TOF_Config::getFileStem(const std::string& section, const std::string& key) const {
    std::string pathStr = getString(section, key); 
    return std::filesystem::path(pathStr).stem().string();
}

std::string GRAMS_TOF_Config::getFileStemWithDir(const std::string& section, const std::string& key) const {
    std::string pathStr = getString(section, key);
    std::filesystem::path p(pathStr);
    return (p.parent_path() / p.stem()).string(); 
}

std::string GRAMS_TOF_Config::getSubDir(const std::string& subDirName) const {
    return (std::filesystem::path(configDir_) / subDirName).string();
}

int GRAMS_TOF_Config::getInt(const std::string& section, const std::string& key) const {
    return std::stoi(getString(section, key));
}

double GRAMS_TOF_Config::getDouble(const std::string& section, const std::string& key) const {
    return std::stod(getString(section, key));
}

const std::string& GRAMS_TOF_Config::getConfigFilePath() const {
    return configFilePath_;
}

