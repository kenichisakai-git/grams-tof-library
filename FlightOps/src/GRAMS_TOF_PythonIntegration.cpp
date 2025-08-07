#include "GRAMS_TOF_PythonIntegration.h"
#include <pybind11/embed.h>
#include "GRAMS_TOF_DAQManager.h"
#include "GRAMS_TOF_Logger.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <exception>
#include <Python.h>

// Only one place for bindings in your project!
PYBIND11_EMBEDDED_MODULE(grams_tof, m) {
    pybind11::class_<GRAMS_TOF_DAQManager>(m, "DAQManager")
        .def(pybind11::init<const std::string&, const std::string&, int, const std::string&, const std::vector<std::string>&>(),
             pybind11::arg("socketPath"),
             pybind11::arg("shmName"),
             pybind11::arg("debugLevel"),
             pybind11::arg("daqType"),
             pybind11::arg("daqCards"));
}

namespace {

class PythonIntegrationImpl {
public:
    explicit PythonIntegrationImpl(GRAMS_TOF_DAQManager& daq) : daq_(daq), guard_()
    {
        namespace py = pybind11;
        try {
            auto globals = py::globals();
            py::module_::import("grams_tof");
            globals["daq"] = py::cast(&daq_, py::return_value_policy::reference);
            Logger::instance().info("[PythonIntegration] Python interpreter initialized and DAQ manager injected.");
        } catch (const std::exception& e) {
            Logger::instance().error("[PythonIntegration] Exception during Python initialization: {}", e.what());
            throw;
        }
    }

    void loadPythonScript(const std::string& filename) {
        namespace py = pybind11;
        try {
            std::ifstream infile(filename);
            if (!infile) {
                Logger::instance().error("[PythonIntegration] Could not open script: {}", filename);
                return;
            }
            std::stringstream buffer;
            buffer << infile.rdbuf();

            py::exec(buffer.str(), py::globals());
            Logger::instance().info("[PythonIntegration] Loaded Python script: {}", filename);
        } catch (const std::exception& e) {
            Logger::instance().error("[PythonIntegration] Exception during script load:", e.what());
        }
    }

    void execPythonFunction(const std::string& func_name) {
        namespace py = pybind11;
        try {
            std::string cmd = func_name + "()";
            Logger::instance().info("[PythonIntegration] Calling Python:: {}", cmd);
            py::eval(cmd, py::globals());
        } catch (const std::exception& e) {
            Logger::instance().error("[PythonIntegration] Exception running {} : {}", func_name, e.what());
        }
    }

    template <typename... Args>
    bool runPythonFunction(const std::string& scriptPath,
                           const std::string& functionName,
                           Args&&... args)
    {
        namespace py = pybind11;
        try {
            auto module = py::module_::import(scriptPath.c_str());
            auto func = module.attr(functionName.c_str());
    
            Logger::instance().info("[PythonIntegration] Calling {}()", functionName);
            if (!func(std::forward<Args>(args)...).template cast<bool>()) {
                Logger::instance().error("[PythonIntegration] {}() returned false.", functionName);
                return false;
            }
            return true;
        } catch (const py::error_already_set& e) {
            Logger::instance().error("[PythonIntegration] Exception in {}(): {}", functionName, e.what());
            PyErr_Clear();
            return false;
        }
    }

    bool runPetsysInitSystem(const std::string& scriptPath) {
        return runPythonFunction(scriptPath, "safe_init_system");
    }
    
    bool runPetsysMakeBiasCalibrationTable(const std::string& scriptPath,
                                           const std::string& outputFile,
                                           const std::vector<int>& portIDs,
                                           const std::vector<int>& slaveIDs,
                                           const std::vector<int>& slotIDs,
                                           const std::vector<std::string>& filenames) {
        namespace py = pybind11;
        py::list py_ports, py_slaves, py_slots, py_files;
        for (auto v : portIDs) py_ports.append(v);
        for (auto v : slaveIDs) py_slaves.append(v);
        for (auto v : slotIDs) py_slots.append(v);
        for (const auto& f : filenames) py_files.append(f);
    
        return runPythonFunction(scriptPath, "safe_make_bias_calibration_table",
                                 outputFile, py_ports, py_slaves, py_slots, py_files);
    }

    bool runPetsysMakeSimpleBiasSettingsTable(const std::string& scriptPath,
                                              const std::string& configPath,
                                              float offset,
                                              float prebd,
                                              float bd,
                                              float over,
                                              const std::string& outputPath) {
        return runPythonFunction(scriptPath, "safe_make_simple_bias_settings_table",
                                 configPath, offset, prebd, bd, over, outputPath);
    }
    
    bool runPetsysMakeSimpleChannelMap(const std::string& scriptPath,
                                       const std::string& outputFile) {
        return runPythonFunction(scriptPath, "safe_make_simple_channel_map", outputFile);
    }

    bool runPetsysMakeSimpleDiscSettingsTable(const std::string& scriptPath,
                                              const std::string& configPath,
                                              int vth_t1,
                                              int vth_t2,
                                              int vth_e,
                                              const std::string& outputPath) {
        return runPythonFunction(scriptPath, "safe_make_simple_disc_settings_table",
                                 configPath, vth_t1, vth_t2, vth_e, outputPath);
    }

    bool runPetsysReadTemperatureSensors(const std::string& scriptPath,
                                         double acq_time = 0.0,
                                         double interval = 60.0,
                                         const std::string& fileName = "/dev/null",
                                         bool startup = false,
                                         bool debug = false) {
        return runPythonFunction(scriptPath, "safe_read_temperature_sensors",
                                 acq_time, interval, fileName, startup, debug);
    }

    bool runPetsysAcquireThresholdCalibration(const std::string& scriptPath,
                                              const std::string& configFile,
                                              const std::string& outFilePrefix,
                                              int noise_reads = 4,
                                              int dark_reads = 4,
                                              bool ext_bias = false) {
        return runPythonFunction(scriptPath, "safe_acquire_threshold_calibration",
                                 configFile, outFilePrefix, noise_reads, dark_reads, ext_bias);
    }

    bool runPetsysAcquireQdcCalibration(const std::string& scriptPath,
                                        const std::string& configPath,
                                        const std::string& fileNamePrefix) {
        return runPythonFunction(scriptPath, "safe_acquire_qdc_calibration",
                                 configPath, fileNamePrefix);
    }

    bool runPetsysAcquireTdcCalibration(const std::string& scriptPath,
                                        const std::string& configPath,
                                        const std::string& fileNamePrefix) {
        return runPythonFunction(scriptPath, "safe_acquire_tdc_calibration",
                                 configPath, fileNamePrefix);
    }

    bool runPetsysAcquireSipmData(const std::string& scriptPath,
                                  const std::string& configPath,
                                  const std::string& fileNamePrefix,
                                  double acquisitionTime,
                                  const std::string& mode,
                                  bool hwTrigger = false,
                                  const std::string& paramTable = "") {
        return runPythonFunction(scriptPath, "safe_acquire_sipm_data",
                                 configPath, fileNamePrefix, acquisitionTime, mode, hwTrigger, paramTable);
    }

private:
    GRAMS_TOF_DAQManager& daq_;
    pybind11::scoped_interpreter guard_;
};

} // namespace

// PImpl class, just a stub that holds our impl (local, unnamed-namespace version)
class GRAMS_TOF_PythonIntegration::Impl {
public:
    explicit Impl(GRAMS_TOF_DAQManager& daq)
        : daq_(daq), impl_(std::make_unique<PythonIntegrationImpl>(daq)) {}
    ~Impl() = default;

    void loadPythonScript(const std::string& filename) {
        impl_->loadPythonScript(filename);
    }
    void execPythonFunction(const std::string& func_name) {
        impl_->execPythonFunction(func_name);
    }
  
    template <typename... Args>
    bool runPythonFunction(const std::string& scriptPath,
                           const std::string& functionName,
                           Args&&... args) {
        return impl_->runPythonFunction(scriptPath, functionName, std::forward<Args>(args)...);
    }
    bool runPetsysInitSystem(const std::string& scriptPath) {
        return impl_->runPetsysInitSystem(scriptPath);
    }
    bool runPetsysMakeBiasCalibrationTable(const std::string& scriptPath,
                                           const std::string& outputFile,
                                           const std::vector<int>& portIDs,
                                           const std::vector<int>& slaveIDs,
                                           const std::vector<int>& slotIDs,
                                           const std::vector<std::string>& filenames) {
        return impl_->runPetsysMakeBiasCalibrationTable(scriptPath, outputFile, portIDs, slaveIDs, slotIDs, filenames);
    }
    bool runPetsysMakeSimpleBiasSettingsTable(const std::string& scriptPath,
                                              const std::string& configPath,
                                              float offset,
                                              float prebd,
                                              float bd,
                                              float over,
                                              const std::string& outputPath) {
        return impl_->runPetsysMakeSimpleBiasSettingsTable(scriptPath, configPath, offset, prebd, bd, over, outputPath);
    }
    bool runPetsysMakeSimpleChannelMap(const std::string& scriptPath,
                                       const std::string& outputFile) {
        return impl_->runPetsysMakeSimpleChannelMap(scriptPath, outputFile);
    }
    bool runPetsysMakeSimpleDiscSettingsTable(const std::string& scriptPath,
                                              const std::string& configPath,
                                              int vth_t1,
                                              int vth_t2,
                                              int vth_e,
                                              const std::string& outputPath) {
        return impl_->runPetsysMakeSimpleDiscSettingsTable(scriptPath, configPath, vth_t1, vth_t2, vth_e, outputPath);
    }
    bool runPetsysReadTemperatureSensors(const std::string& scriptPath,
                                         double acq_time,
                                         double interval,
                                         const std::string& fileName,
                                         bool startup,
                                         bool debug) {
        return impl_->runPetsysReadTemperatureSensors(scriptPath, acq_time, interval, fileName, startup, debug);
    }
    
    bool runPetsysAcquireThresholdCalibration(const std::string& scriptPath,
                                              const std::string& configFile,
                                              const std::string& outFilePrefix,
                                              int noise_reads,
                                              int dark_reads,
                                              bool ext_bias) {
        return impl_->runPetsysAcquireThresholdCalibration(scriptPath, configFile, outFilePrefix,
                                                           noise_reads, dark_reads, ext_bias);
    }
    
    bool runPetsysAcquireQdcCalibration(const std::string& scriptPath,
                                        const std::string& configPath,
                                        const std::string& fileNamePrefix) {
        return impl_->runPetsysAcquireQdcCalibration(scriptPath, configPath, fileNamePrefix);
    }
    
    bool runPetsysAcquireTdcCalibration(const std::string& scriptPath,
                                        const std::string& configPath,
                                        const std::string& fileNamePrefix) {
        return impl_->runPetsysAcquireTdcCalibration(scriptPath, configPath, fileNamePrefix);
    }
    
    bool runPetsysAcquireSipmData(const std::string& scriptPath,
                                  const std::string& configPath,
                                  const std::string& fileNamePrefix,
                                  double acquisitionTime,
                                  const std::string& mode,
                                  bool hwTrigger,
                                  const std::string& paramTable) {
        return impl_->runPetsysAcquireSipmData(scriptPath, configPath, fileNamePrefix,
                                               acquisitionTime, mode, hwTrigger, paramTable);
    } 

    GRAMS_TOF_DAQManager& daq_;
    GRAMS_TOF_DAQManager& getDAQ() { return daq_; }
 
private:
    std::unique_ptr<PythonIntegrationImpl> impl_;
};

// ---- Interface implementation wraps the real implementation ----
GRAMS_TOF_PythonIntegration::GRAMS_TOF_PythonIntegration(GRAMS_TOF_DAQManager& daq)
    : impl_(std::make_unique<Impl>(daq)) {
    // Set the special flag to prevent __main__ in scripts
    PyRun_SimpleString("import sys; sys._called_from_c = True");

    // If you also want to load scripts automatically, you can do it here too:
    loadPythonScript("scripts/init_system.py");
    loadPythonScript("scripts/make_bias_calibration_table.py");
    loadPythonScript("scripts/make_simple_bias_settings_table.py");
    loadPythonScript("scripts/make_simple_channel_map.py");
    loadPythonScript("scripts/make_simple_disc_settings_table.py");
    loadPythonScript("scripts/read_temperature_sensors.py");
    loadPythonScript("scripts/acquire_threshold_calibration.py");
    loadPythonScript("scripts/acquire_qdc_calibration.py");
    loadPythonScript("scripts/acquire_tdc_calibration.py");
    loadPythonScript("scripts/acquire_sipm_data.py");
}

GRAMS_TOF_PythonIntegration::~GRAMS_TOF_PythonIntegration() = default;

void GRAMS_TOF_PythonIntegration::loadPythonScript(const std::string& filename) {
    impl_->loadPythonScript(filename);
}

void GRAMS_TOF_PythonIntegration::execPythonFunction(const std::string& func_name) {
    impl_->execPythonFunction(func_name);
}

template <typename... Args>
bool GRAMS_TOF_PythonIntegration::runPythonFunction(const std::string& scriptPath,
                       const std::string& functionName,
                       Args&&... args) {
   return impl_->runPythonFunction(scriptPath, functionName, std::forward<Args>(args)...);
}

bool GRAMS_TOF_PythonIntegration::runPetsysInitSystem(const std::string& scriptPath) {
    return impl_->runPetsysInitSystem(scriptPath);
}

bool GRAMS_TOF_PythonIntegration::runPetsysMakeBiasCalibrationTable(
    const std::string& scriptPath,
    const std::string& outputFile,
    const std::vector<int>& portIDs,
    const std::vector<int>& slaveIDs,
    const std::vector<int>& slotIDs,
    const std::vector<std::string>& filenames) {
    return impl_->runPetsysMakeBiasCalibrationTable(scriptPath, outputFile, portIDs, slaveIDs, slotIDs, filenames);
}

bool GRAMS_TOF_PythonIntegration::runPetsysMakeSimpleBiasSettingsTable(
    const std::string& scriptPath,
    const std::string& configPath,
    float offset,
    float prebd,
    float bd,
    float over,
    const std::string& outputPath) {
    return impl_->runPetsysMakeSimpleBiasSettingsTable(scriptPath, configPath, offset, prebd, bd, over, outputPath);
}

bool GRAMS_TOF_PythonIntegration::runPetsysMakeSimpleChannelMap(const std::string& scriptPath,
                                                                const std::string& outputFile) {
    return impl_->runPetsysMakeSimpleChannelMap(scriptPath, outputFile);
}

bool GRAMS_TOF_PythonIntegration::runPetsysMakeSimpleDiscSettingsTable(
    const std::string& scriptPath,
    const std::string& configPath,
    int vth_t1,
    int vth_t2,
    int vth_e,
    const std::string& outputPath) {
   return impl_->runPetsysMakeSimpleDiscSettingsTable(scriptPath, configPath, vth_t1, vth_t2, vth_e, outputPath);
}

bool GRAMS_TOF_PythonIntegration::runPetsysReadTemperatureSensors(
    const std::string& scriptPath,
    double acq_time,
    double interval,
    const std::string& fileName,
    bool startup,
    bool debug) {
    return impl_->runPetsysReadTemperatureSensors(scriptPath, acq_time, interval, fileName, startup, debug);
}

bool GRAMS_TOF_PythonIntegration::runPetsysAcquireThresholdCalibration(
    const std::string& scriptPath,
    const std::string& configFile,
    const std::string& outFilePrefix,
    int noise_reads,
    int dark_reads,
    bool ext_bias) {
    return impl_->runPetsysAcquireThresholdCalibration(scriptPath, configFile, outFilePrefix,
                                                       noise_reads, dark_reads, ext_bias);
}

bool GRAMS_TOF_PythonIntegration::runPetsysAcquireQdcCalibration(
    const std::string& scriptPath,
    const std::string& configPath,
    const std::string& fileNamePrefix) {
    return impl_->runPetsysAcquireQdcCalibration(scriptPath, configPath, fileNamePrefix);
}

bool GRAMS_TOF_PythonIntegration::runPetsysAcquireTdcCalibration(
    const std::string& scriptPath,
    const std::string& configPath,
    const std::string& fileNamePrefix) {
    return impl_->runPetsysAcquireTdcCalibration(scriptPath, configPath, fileNamePrefix);
}

bool GRAMS_TOF_PythonIntegration::runPetsysAcquireSipmData(
    const std::string& scriptPath,
    const std::string& configPath,
    const std::string& fileNamePrefix,
    double acquisitionTime,
    const std::string& mode,
    bool hwTrigger,
    const std::string& paramTable) {
    return impl_->runPetsysAcquireSipmData(scriptPath, configPath, fileNamePrefix,
                                           acquisitionTime, mode, hwTrigger, paramTable);
}

GRAMS_TOF_DAQManager& GRAMS_TOF_PythonIntegration::getDAQ() {
    return impl_->getDAQ();
}


