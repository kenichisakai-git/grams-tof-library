#include "GRAMS_TOF_DAQManager.h"
#include "GRAMS_TOF_PythonIntegration.h"
#include "GRAMS_TOF_CommandServer.h"
#include "GRAMS_TOF_CommandDefs.h"
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
    GRAMS_TOF_CommandServer server(
        12345,
        [&](const GRAMS_TOF_CommandCodec::Packet& pkt) { 
            TOFCommandCode code = pkt.code;
            const auto& argv = pkt.argv;

            switch (code) {
                case TOFCommandCode::START_DAQ:
                    Logger::instance().warn("[CommandServer] Starting DAQ..."); 
                    daq.run(); 
                    break;

                case TOFCommandCode::STOP_DAQ:
                    Logger::instance().warn("[CommandServer] Stoping DAQ..."); 
                    daq.stop(); 
                    break;

                case TOFCommandCode::RESET_DAQ:
                    Logger::instance().warn("[CommandServer] Resetting DAQ..."); 
                    daq.reset(); 
                    break;

                case TOFCommandCode::RUN_INIT_SYS:
                    Logger::instance().warn("[CommandServer] Executing init_system.py script..."); 
                    pyint.runPetsysInitSystem(
                        "scripts.init_system"  // module name
                    );
                    break;

                case TOFCommandCode::RUN_BIAS_CAL:
                    Logger::instance().warn("[CommandServer] Executing make_bias_calibration_table.py script..."); 
                    pyint.runPetsysMakeBiasCalibrationTable(
                        "scripts.make_bias_calibration_table",  // module name 
                        "/tmp/bias_output.txt",                 // output file
                        {argv.size() > 0 ? argv[0] : 0},        // portIDs.
                        {argv.size() > 1 ? argv[1] : 0},        // slaveIDs
                        {argv.size() > 2 ? argv[2] : 0},        // slotIDs
                        {}                                      // no calibration files
                    );
                    break;

                case TOFCommandCode::RUN_BIAS_SET:
                    Logger::instance().warn("[CommandServer] Executing make_simple_bias_settings_table.py script..."); 
                    pyint.runPetsysMakeSimpleBiasSettingsTable(
                        "scripts.make_simple_bias_settings_table", // module name 
                        "/tmp/config.dat",                         // Configuration file
                        static_cast<float>(argv.size() > 0 ? argv[0] : 0),  // Bias channel offset
                        static_cast<float>(argv.size() > 1 ? argv[1] : 0),  // Pre-breakdown voltage
                        static_cast<float>(argv.size() > 2 ? argv[2] : 0),  // Breakdown voltage
                        static_cast<float>(argv.size() > 3 ? argv[3] : 0),  // Nominal overvoltage
                        {}                                                  // Output file
                    );
                    break;

                case TOFCommandCode::RUN_CHAN_MAP:
                    Logger::instance().warn("[CommandServer] Executing make_simple_channel_map.py script..."); 
                    pyint.runPetsysMakeSimpleChannelMap(
                        "scripts.make_simple_channel_map", // module name 
                        {}                                 // Output file
                    );
                    break;

                case TOFCommandCode::RUN_DISC_SET:
                    Logger::instance().warn("[CommandServer] Executing make_simple_disc_settings_table.py script..."); 
                    pyint.runPetsysMakeSimpleDiscSettingsTable(
                        "scripts.make_simple_disc_settings_table", // module name 
                        "/tmp/config.dat",                         // Configuration file
                        {argv.size() > 0 ? argv[0] : 0},           // Discriminator T1 (DAC above zero)
                        {argv.size() > 1 ? argv[1] : 0},           // Discriminator T2 (DAC above zero)
                        {argv.size() > 2 ? argv[2] : 0},           // Discriminator E (DAC above zero)
                        {}                                         // Output file
                    );
                    break;

              default:
               Logger::instance().error("[CommandServer] Unknown or unhandled command code: 0x{:X}", code);
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

