#include "GRAMS_TOF_Config.h"
#include "GRAMS_TOF_Logger.h"
#include "INIReader.h"

#include <cstdlib>
#include <stdexcept>
#include <filesystem>
#include <iostream>
#include <regex>
#include <vector>
#include <algorithm>

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
        std::cerr << "Error: GLIB environment variable not set. Default config not loaded." << std::endl;
        return false;
    }

    const char* tofdata = std::getenv("TOFDATA");
    if (!tofdata) {
        std::cerr << "Error: TOFDATA environment variable is not set. Cannot continue." << std::endl;
        return false;
    } else {
        config.tofdataDir_ = tofdata;
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

    if ((pos = result.find("%CDIR%")) != std::string::npos) result.replace(pos, 6, configDir_);
    if ((pos = result.find("%PWD%")) != std::string::npos) result.replace(pos, 5, ".");
    if ((pos = result.find("%HOME%")) != std::string::npos) {
        const char* home = std::getenv("HOME");
        if (home) result.replace(pos, 6, home);
    }
    if ((pos = result.find("$TOFDATA")) != std::string::npos) {
        const char* tofdata = std::getenv("TOFDATA");
        if (tofdata) result.replace(pos, 8, tofdata);
        else throw std::runtime_error("TOFDATA environment variable not set.");
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

std::string GRAMS_TOF_Config::getAbsolutePath(const std::string& section, const std::string& key) const {
    std::string pathStr = getString(section, key);
    std::filesystem::path p(pathStr);
    return std::filesystem::absolute(p).string();
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

std::string GRAMS_TOF_Config::getFileByTimestamp(
    const std::string& absDir,
    const std::string& prefix,
    const std::string& timestamp,
    const std::string& suffix) const
{
    namespace fs = std::filesystem;

    fs::path dir(absDir);
    if (!fs::exists(dir) || !fs::is_directory(dir))
        throw std::runtime_error("Directory does not exist: " + dir.string());

    std::string pattern = prefix + "_" + timestamp + suffix + R"(.*)";
    std::regex re(pattern);

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;

        const std::string filename = entry.path().filename().string();
        if (std::regex_match(filename, re))
            return entry.path().string();
    }

    throw std::runtime_error(
        "No file found for timestamp " + timestamp + 
        " with suffix '" + suffix + "' in " + dir.string()
    );
}


std::string GRAMS_TOF_Config::makeFilePathWithTimestamp(
    const std::string& absDir,
    const std::string& prefix,
    const std::string& timestamp,
    const std::string& ext ) const
{
    namespace fs = std::filesystem;
    fs::path dir(absDir);
    if (!fs::exists(dir)) fs::create_directories(dir);

    std::string dotExt = ext.empty() ? "" : (ext.front() == '.' ? ext : "." + ext);

    return (dir / (prefix + "_" + timestamp + dotExt)).string();
}


std::string GRAMS_TOF_Config::getLatestTimestamp(
    const std::string& absDir,
    const std::string& prefix,
    const std::string& suffix) const 
{
    namespace fs = std::filesystem;

    fs::path dir(absDir);
    if (!fs::exists(dir) || !fs::is_directory(dir))
        throw std::runtime_error("Directory does not exist: " + dir.string());

    std::regex re(prefix + R"(_(\d{4}-\d{2}-\d{2}_\d{2}-\d{2}-\d{2}\.\d+Z))" + suffix + R"(.*)");

    std::string latestTs;

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;

        std::smatch match;
        std::string filename = entry.path().filename().string();
        
        if (std::regex_match(filename, match, re)) {
            const std::string& ts = match[1].str();
            if (latestTs.empty() || ts > latestTs) {
                latestTs = ts;
            }
        }
    }

    if (latestTs.empty()) {
        throw std::runtime_error("No matching file found with suffix '" + suffix + "' in " + dir.string());
    }

    return latestTs;
}


std::string GRAMS_TOF_Config::getCurrentTimestamp() const
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto secs = time_point_cast<std::chrono::seconds>(now);
    auto frac = duration_cast<milliseconds>(now - secs).count();

    std::time_t t = system_clock::to_time_t(secs);
    std::tm tm = *std::gmtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S")
        << "." << frac << "Z";
    return oss.str();
}

void GRAMS_TOF_Config::copyOrLink(const std::string& srcPath, const std::string& dstPath, bool symlink) const
{
    namespace fs = std::filesystem;
    fs::path src(srcPath);
    fs::path dst(dstPath);

    Logger::instance().info(
        "[GRAMS_TOF_Config] copyOrLink: src='{}', dst='{}', mode={}",
        src.string(),
        dst.string(),
        symlink ? "symlink" : "copy"
    );

    if (!fs::exists(src)) throw std::runtime_error("Source file does not exist: " + src.string());
    if (fs::exists(dst)) fs::remove(dst);  

    if (symlink) {
        fs::create_symlink(src, dst);
    } else {
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
    }
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

