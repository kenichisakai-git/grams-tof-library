#include "GRAMS_TOF_DAQManager.h"
#include "GRAMS_TOF_PythonIntegration.h"
#include "GRAMS_TOF_CommandServer.h" // from prior message

int main() {
    GRAMS_TOF_DAQManager daq(
        "/tmp/d.sock",        // socketPath
        "/daqd_shm",          // shmName
        0,                    // debugLevel
        "GBE",                // daqType
        {"/dev/psdaq0"}       // daqCards
    );

    daq.initialize();
    daq.run();

    /*
    GRAMS_TOF_PythonIntegration pyint(daq);
    pyint.loadPythonScript("python/sample1.py");
    pyint.loadPythonScript("python/sample2.py");

    GRAMS_TOF_CommandServer server(
        12345,
        [&](const std::string& command) {
            pyint.runUserScriptByCommand(command);
        }
    );
    server.start();

    std::cout << "[System] Waiting for commands on port 12345. Press Enter to quit.\n";
    std::cin.get();
    server.stop();
    std::cout << "[System] Exiting.\n";
    */

    return 0;
}

