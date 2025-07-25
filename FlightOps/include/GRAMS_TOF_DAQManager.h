// GRAMS_TOF_DAQManager.h
#pragma once

#include "DAQFrameServer.h"

#include <cstring>
#include <vector>

class GRAMS_TOF_DAQManager {
public:
    GRAMS_TOF_DAQManager(const std::string& socketPath, const std::string& shmName, int debugLevel, const std::string& daqType, const std::vector<std::string>& daqCards);
    ~GRAMS_TOF_DAQManager();

    bool initialize();
    bool run();  // launches poll loop
    void stop();
    
    bool startAcquisition();
    bool stopAcquisition();
    void setBias(int voltage);

private:
    std::string socketPath_;
    std::string shmName_;
    int debugLevel_;
    std::string daqType_;
    std::vector<std::string> daqCardList_;
    unsigned daqCardPortBits_;

    int clientSocket_;
    int shmfd_;
    PETSYS::RawDataFrame* shmPtr_;
    PETSYS::FrameServer* frameServer_;
    std::vector<PETSYS::AbstractDAQCard*> daqCards_;
    
    bool is_acq_running_;

    int createListeningSocket();
    void pollSocket();  // epoll loop
    void cleanup();
};

