#include "GRAMS_TOF_DAQManager.h"
#include "GRAMS_TOF_PythonIntegration.h"
#include "GRAMS_TOF_CommandServer.h"
#include "GRAMS_TOF_CommandDefs.h"

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
        [&](const GRAMS_TOF_CommandCodec::Packet& pkt) { 
            TOFCommandCode code = pkt.code;
            const auto& argv = pkt.argv;

            switch (code) {
                case TOFCommandCode::START_DAQ:
                    std::cout << "[CommandServer] Starting DAQ...\n";
                    daq.run(); // Hypothetical
                    break;

                case TOFCommandCode::RUN_BIAS_CAL:
                    std::cout << "[CommandServer] Executing bias calibration script...\n";
                    pyint.runMakeBiasCalibrationTable(
                        "scripts.make_bias_calibration_table",  // module name (not path)
                        "/tmp/bias_output.txt",                 // output file
                        {argv.size() > 0 ? argv[0] : 0},        // portIDs.
                        {argv.size() > 1 ? argv[1] : 0},        // slaveIDs
                        {argv.size() > 2 ? argv[2] : 0},        // slotIDs
                        {}                                      // no calibration files
                  );
                  break;

              default:
                std::cerr << "[CommandServer] Unknown or unhandled command code: " << static_cast<uint16_t>(code) << "\n";
            }
        }
    );
    server.start();

    std::cout << "[System] Running DAQ and waiting for commands on port 12345.\n";
    std::cout << "[System] Press Enter to quit.\n";
    //daq.run(); 

    std::cout << "[System] Waiting for commands on port 12345. Press Enter to quit.\n";
    std::cin.get();
    server.stop();
    std::cout << "[System] Exiting.\n";

    return 0;
}

