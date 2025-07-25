#include "GRAMS_TOF_PythonIntegration.h"
#include <pybind11/embed.h>
#include "GRAMS_TOF_DAQManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <exception>

// Only one place for bindings in your project!
PYBIND11_EMBEDDED_MODULE(grams_tof, m) {
    pybind11::class_<GRAMS_TOF_DAQManager>(m, "DAQManager")
        .def(pybind11::init<const std::string&, const std::string&, int, const std::string&, const std::vector<std::string>&>(),
             pybind11::arg("socketPath"),
             pybind11::arg("shmName"),
             pybind11::arg("debugLevel"),
             pybind11::arg("daqType"),
             pybind11::arg("daqCards"))
        .def("startAcquisition", &GRAMS_TOF_DAQManager::startAcquisition)
        .def("stopAcquisition", &GRAMS_TOF_DAQManager::stopAcquisition)
        .def("setBias", &GRAMS_TOF_DAQManager::setBias);
}

// **** BEGIN UNNAMED NAMESPACE IMPL ****
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

    // Helper: Map command ("RUN1") to script function ("user_script1")
    bool runUserScriptByCommand(const std::string& cmd) {
        std::string func;
        if      (cmd == "RUN1") func = "user_script1";
        else if (cmd == "RUN2") func = "user_script2";
        else if (cmd == "RUN3") func = "user_script3";  // Add more as needed!
        else {
            std::cerr << "[PythonIntegration] Unknown command: " << cmd << std::endl;
            return false;
        }
        execPythonFunction(func);
        return true;
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
    bool runUserScriptByCommand(const std::string& cmd) {
        return impl_->runUserScriptByCommand(cmd);
    }

private:
    std::unique_ptr<PythonIntegrationImpl> impl_;
};

// ---- Interface implementation wraps the real implementation ----
GRAMS_TOF_PythonIntegration::GRAMS_TOF_PythonIntegration(GRAMS_TOF_DAQManager& daq)
    : impl_(std::make_unique<Impl>(daq)) {}

GRAMS_TOF_PythonIntegration::~GRAMS_TOF_PythonIntegration() = default;

void GRAMS_TOF_PythonIntegration::loadPythonScript(const std::string& filename) {
    impl_->loadPythonScript(filename);
}

void GRAMS_TOF_PythonIntegration::execPythonFunction(const std::string& func_name) {
    impl_->execPythonFunction(func_name);
}

bool GRAMS_TOF_PythonIntegration::runUserScriptByCommand(const std::string& cmd) {
    return impl_->runUserScriptByCommand(cmd);
}
