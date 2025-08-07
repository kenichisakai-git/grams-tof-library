#pragma once

#include <string>
#include <unordered_map>

class GRAMS_TOF_Config {
public:
    bool load(const std::string& filename);

    std::string getString(const std::string& section, const std::string& key) const;
    int getInt(const std::string& section, const std::string& key) const;
    double getDouble(const std::string& section, const std::string& key) const;

private:
    using Section = std::unordered_map<std::string, std::string>;
    std::unordered_map<std::string, Section> data_;
    std::string configDir_;

    std::string substituteVariables(const std::string& value) const;
};

