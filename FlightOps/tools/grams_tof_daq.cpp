#include "GRAMS_TOF_DAQManager.h"
#include "GRAMS_TOF_PythonIntegration.h"
#include "GRAMS_TOF_CommandServer.h"

int main() {
    GRAMS_TOF_DAQManager daq(
        "/tmp/d.sock",        // socketPath
        "/daqd_shm",          // shmName
        0,                    // debugLevel
        "GBE",                // daqType
        {"/dev/psdaq0"}       // daqCards
    );

    if (!daq.initialize()) {
        std::cerr << "[System] DAQ initialization failed.\n";
        //return 1;
    }

    GRAMS_TOF_PythonIntegration pyint(daq);
    pyint.loadPythonScript("scripts/make_bias_calibration_table.py"); 

    GRAMS_TOF_CommandServer server(
        12345,
        [&](const std::string& command) {
            if (command == "RUN_BIAS_CAL") {
                std::cout << "[CommandServer] Executing bias calibration script...\n";
                pyint.runMakeBiasCalibrationTable(
                    "scripts.make_bias_calibration_table",  // module name (not path)
                    "/tmp/bias_output.txt",                 // output file
                    {0}, {0}, {0},                          // portIDs, slaveIDs, slotIDs
                    {}                                      // no calibration files
                );
            }
        }
    );
    server.start();

    std::cout << "[System] Running DAQ and waiting for commands on port 12345.\n";
    std::cout << "[System] Press Enter to quit.\n";
    daq.run(); 

    std::cout << "[System] Waiting for commands on port 12345. Press Enter to quit.\n";
    std::cin.get();
    server.stop();
    std::cout << "[System] Exiting.\n";

    return 0;
}

