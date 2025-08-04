#include "GRAMS_TOF_PythonIntegration.h"
#include <pybind11/embed.h>
#include "GRAMS_TOF_DAQManager.h"
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
            std::cout << "[PythonIntegration] Python interpreter initialized and DAQ manager injected.\n";
        } catch (const std::exception& e) {
            std::cerr << "[PythonIntegration] Exception during Python initialization: " << e.what() << std::endl;
            throw;
        }
    }

    void loadPythonScript(const std::string& filename) {
        namespace py = pybind11;
        try {
            std::ifstream infile(filename);
            if (!infile) {
                std::cerr << "[PythonIntegration] Could not open script: " << filename << std::endl;
                return;
            }
            std::stringstream buffer;
            buffer << infile.rdbuf();

            py::exec(buffer.str(), py::globals());
            std::cout << "[PythonIntegration] Loaded Python script: " << filename << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[PythonIntegration] Exception during script load: " << e.what() << std::endl;
        }
    }

    void execPythonFunction(const std::string& func_name) {
        namespace py = pybind11;
        try {
            std::string cmd = func_name + "()";
            std::cout << "[PythonIntegration] Calling Python: " << cmd << std::endl;
            py::eval(cmd, py::globals());
        } catch (const std::exception& e) {
            std::cerr << "[PythonIntegration] Exception running " << func_name << ": " << e.what() << std::endl;
        }
    }

    bool runPetsysInitSystem(const std::string& scriptPath) {
        namespace py = pybind11;
        try {
            auto mbct = py::module_::import(scriptPath.c_str());
            auto func = mbct.attr("init_system");

            std::cout << "[PythonIntegration] Calling init_system()\n";
            func();
            return true;
        } catch (const py::error_already_set &e) {
            //std::cerr << "[PythonIntegration] Python exception what(): " << e.what() << std::endl;
            std::cerr << "[PythonIntegration] Python exception what() " << std::endl;
            PyErr_Clear();
            return false;
        }
    }

    bool runPetsysMakeBiasCalibrationTable(const std::string& scriptPath,
                                           const std::string& outputFile,
                                           const std::vector<int>& portIDs,
                                           const std::vector<int>& slaveIDs,
                                           const std::vector<int>& slotIDs,
                                           const std::vector<std::string>& filenames) {
        namespace py = pybind11;
        try {
            auto mbct = py::module_::import(scriptPath.c_str());
            auto func = mbct.attr("make_bias_calibration_table");

            py::list py_ports, py_slaves, py_slots, py_files;
            for (auto v : portIDs) py_ports.append(v);
            for (auto v : slaveIDs) py_slaves.append(v);
            for (auto v : slotIDs) py_slots.append(v);
            for (auto& f : filenames) py_files.append(f);

            std::cout << "[PythonIntegration] Calling make_bias_calibration_table()\n";
            func(outputFile, py_ports, py_slaves, py_slots, py_files);
            return true;
        //} catch (const std::exception& e) {
        } catch (const py::error_already_set &e) {
            //std::cerr << "[PythonIntegration] Python exception what(): '" << e.what() << "'" << std::endl;
            std::cerr << "[PythonIntegration] Python exception what(): " << std::endl;
            PyErr_Clear();
            return false;
        }
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
        : impl_(std::make_unique<PythonIntegrationImpl>(daq)) {}
    ~Impl() = default;

    void loadPythonScript(const std::string& filename) {
        impl_->loadPythonScript(filename);
    }
    void execPythonFunction(const std::string& func_name) {
        impl_->execPythonFunction(func_name);
    }
    bool runPetsysInitSystem(const std::string& scriptModule) {
        return impl_->runPetsysInitSystem(scriptModule);
    }
    bool runPetsysMakeBiasCalibrationTable(const std::string& scriptModule,
                                           const std::string& outputFile,
                                           const std::vector<int>& portIDs,
                                           const std::vector<int>& slaveIDs,
                                           const std::vector<int>& slotIDs,
                                           const std::vector<std::string>& filenames) {
        return impl_->runPetsysMakeBiasCalibrationTable(scriptModule, outputFile, portIDs, slaveIDs, slotIDs, filenames);
    }

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
}

GRAMS_TOF_PythonIntegration::~GRAMS_TOF_PythonIntegration() = default;

void GRAMS_TOF_PythonIntegration::loadPythonScript(const std::string& filename) {
    impl_->loadPythonScript(filename);
}

void GRAMS_TOF_PythonIntegration::execPythonFunction(const std::string& func_name) {
    impl_->execPythonFunction(func_name);
}

bool GRAMS_TOF_PythonIntegration::runPetsysInitSystem(const std::string& scriptModule) {
    return impl_->runPetsysInitSystem(scriptModule);
}

bool GRAMS_TOF_PythonIntegration::runPetsysMakeBiasCalibrationTable(
    const std::string& scriptModule,
    const std::string& outputFile,
    const std::vector<int>& portIDs,
    const std::vector<int>& slaveIDs,
    const std::vector<int>& slotIDs,
    const std::vector<std::string>& filenames) {
    return impl_->runPetsysMakeBiasCalibrationTable(scriptModule, outputFile, portIDs, slaveIDs, slotIDs, filenames);
}

