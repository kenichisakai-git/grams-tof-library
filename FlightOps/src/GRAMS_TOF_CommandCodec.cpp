#include "GRAMS_TOF_CommandCodec.h"
#include "GRAMS_TOF_Logger.h"
#include <iostream>

static uint16_t read16(const uint8_t* ptr) {
    return ptr[0] | (ptr[1] << 8);
}

static int32_t read32(const uint8_t* ptr) {
    return ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
}

static void write16(std::vector<uint8_t>& buf, uint16_t val) {
    buf.push_back(val & 0xFF);
    buf.push_back((val >> 8) & 0xFF);
}

static void write32(std::vector<uint8_t>& buf, int32_t val) {
    buf.push_back(val & 0xFF);
    buf.push_back((val >> 8) & 0xFF);
    buf.push_back((val >> 16) & 0xFF);
    buf.push_back((val >> 24) & 0xFF);
}

bool GRAMS_TOF_CommandCodec::parse(const std::vector<uint8_t>& data, Packet& outPacket) {
    if (data.size() < 12) return false;

    const uint8_t* ptr = data.data();
    uint16_t header1 = read16(ptr);
    uint16_t header2 = read16(ptr + 2);
    if (header1 != 0xEB90 || header2 != 0x5B6A) {
        Logger::instance().error("[Codec] Invalid header");
        return false;
    }

    uint16_t code = read16(ptr + 4);
    uint16_t argc = read16(ptr + 6);
    size_t payloadSize  = 8 + argc * 4;            // [Headers + code + argc + argv]
    size_t expectedSize = payloadSize + 2 + 2 + 2; // + CRC + Footer1 + Footer2

    if (data.size() < expectedSize) {
        Logger::instance().error("[Codec] Packet too short for argc = {}", argc);
        return false;
    }

    // CRC check
    uint16_t receivedCRC = read16(ptr + payloadSize);
    uint16_t computedCRC = computeCRC16_CCITT_8408(ptr, payloadSize);
    if (receivedCRC != computedCRC) {
        Logger::instance().error("[Codec] CRC mismatch. Received 0x{:X}, Expected 0x{:X}", receivedCRC, computedCRC);
        return false;
    }

    uint16_t footer1 = read16(ptr + payloadSize + 2);
    uint16_t footer2 = read16(ptr + payloadSize + 4);
    if (footer1 != 0xC5A4 || footer2 != 0xD279) {
        Logger::instance().error("[Codec] Invalid footer");
        return false;
    }
        
    Logger::instance().detail("[Parser] Parsed code = 0x{:X}, argc = {}", code, argc);
    outPacket.code = static_cast<TOFCommandCode>(code);
    outPacket.argc = argc;
    outPacket.argv.clear();
    for (size_t i = 0; i < argc; ++i) {
        uint32_t arg = read32(ptr + 8 + i * 4);
        outPacket.argv.push_back(arg);
    }

    return true;
}

std::vector<uint8_t> GRAMS_TOF_CommandCodec::serialize(const Packet& pkt) {
    std::vector<uint8_t> buf;

    write16(buf, 0xEB90); // Header 1
    write16(buf, 0x5B6A); // Header 2
    write16(buf, static_cast<uint16_t>(pkt.code));
    //write16(buf, static_cast<uint16_t>(pkt.argv.size()));
    write16(buf, static_cast<uint16_t>(pkt.argc)); 

    //for (int32_t arg : pkt.argv) 
    //    write32(buf, arg);
   
    for (size_t i = 0; i < pkt.argc; ++i)
        write32(buf, pkt.argv[i]);
 
    // Compute CRC of everything so far
    uint16_t crc = computeCRC16_CCITT_8408(buf.data(), buf.size());
    write16(buf, crc);    // CRC (Temporary) 

    write16(buf, 0xC5A4); // Footer 1
    write16(buf, 0xD279); // Footer 2

    return buf;
}

//CRC-16-CCITT (0x1021) Just temporary
uint16_t GRAMS_TOF_CommandCodec::computeCRC16_CCITT_1021(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int j = 0; j < 8; ++j)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
    }
    return crc;
}

// CRC-16-CCITT (0x8408, LSB-first) per Shota's definition
uint16_t GRAMS_TOF_CommandCodec::computeCRC16_CCITT_8408(const uint8_t* data, size_t len) {
    uint16_t crc = 0x0000;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 1)  crc = (crc >> 1) ^ 0x8408;
            else          crc >>= 1;
        }
    }
    return crc;
}


