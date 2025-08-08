#include "GRAMS_TOF_DAQManager.h"
#include "GRAMS_TOF_PythonIntegration.h"
#include "GRAMS_TOF_Analyzer.h"
#include "GRAMS_TOF_CommandServer.h"
#include "GRAMS_TOF_CommandDefs.h"
#include "GRAMS_TOF_CommandDispatch.h"
#include "GRAMS_TOF_Logger.h"

int main() {
    Logger::instance().setLogFile("log/daq_log.txt");

    GRAMS_TOF_DAQManager daq(
        "/tmp/d.sock",        // socketPath
        "/daqd_shm",          // shmName
        0,                    // debugLevel
        "GBE",                // daqType
        {"/dev/psdaq0"}       // daqCards
    );

    /*
    if (!daq.initialize()) {
        std::cerr << "[System] DAQ initialization failed.\n";
        return 1;
    }
    */

    setenv("DEBUG", "1", 1); 
    GRAMS_TOF_PythonIntegration pyint(daq);
    GRAMS_TOF_Analyzer analyzer;
    GRAMS_TOF_CommandDispatch dispatchTable(pyint, analyzer);

    GRAMS_TOF_CommandServer server(
        12345,
        [&](const GRAMS_TOF_CommandCodec::Packet& pkt) { 
            TOFCommandCode code = pkt.code;
            const auto& argv = pkt.argv;

            if (!dispatchTable.dispatch(code, argv)) {
                Logger::instance().error("[CommandServer] Command failed or unknown: {}", code);
            }
        }
    );
    server.start();

    Logger::instance().info("[System] Running DAQ and waiting for commands on port 12345");
    Logger::instance().info("[System] Press Enter to quit");
    //daq.run(); 

    Logger::instance().info("[System] Waiting for commands on port 12345. Press Enter to quit");
    std::cin.get();
    server.stop();
    Logger::instance().info("[System] Exiting");

    return 0;
}

