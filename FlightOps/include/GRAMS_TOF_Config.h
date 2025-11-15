#pragma once

#include <string>
#include <unordered_map>

class GRAMS_TOF_Config {
public:
    static GRAMS_TOF_Config& instance();

    void setConfigFile(const std::string& filename);

    std::string getString(const std::string& section, const std::string& key) const;
    std::string getFileStem(const std::string& section, const std::string& key) const;
    std::string getFileStemWithDir(const std::string& section, const std::string& key) const;
    std::string getSubDir(const std::string& subDirName) const;
    int getInt(const std::string& section, const std::string& key) const;
    double getDouble(const std::string& section, const std::string& key) const;

    const std::string& getConfigFilePath() const;

private:
    GRAMS_TOF_Config() = default;
    GRAMS_TOF_Config(const GRAMS_TOF_Config&) = delete;
    GRAMS_TOF_Config& operator=(const GRAMS_TOF_Config&) = delete;

    bool load(const std::string& filename);

    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> data_;
    std::string configDir_;
    std::string configFilePath_;

    std::string substituteVariables(const std::string& value) const;
};

