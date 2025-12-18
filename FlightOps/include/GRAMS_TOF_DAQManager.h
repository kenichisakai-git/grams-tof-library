#pragma once

#include "DAQFrameServer.h"
#include "Client.h"
#include "GRAMS_TOF_FDManager.h"

#include <string>
#include <vector>
#include <map>
#include <memory>

class GRAMS_TOF_DAQManager {
public:
    GRAMS_TOF_DAQManager(
        const std::string& socketPath,
        const std::string& shmName,
        int debugLevel,
        const std::string& daqType,
        const std::vector<std::string>& daqCards);

    ~GRAMS_TOF_DAQManager();

    bool initialize();
    bool run();
    void stop();
    void cleanup();

private:
    int createListeningSocket();
    void pollSocket();

private:
    std::string socketPath_;
    std::string shmName_;
    int debugLevel_;
    std::string daqType_;
    std::vector<std::string> daqCardList_;
    unsigned daqCardPortBits_;

    int shmfd_;
    PETSYS::RawDataFrame* shmPtr_;
    PETSYS::FrameServer* frameServer_;
    std::vector<PETSYS::AbstractDAQCard*> daqCards_;
    bool is_acq_running_;

    // FDs are now managed via GRAMS_TOF_FDManager singleton
};

