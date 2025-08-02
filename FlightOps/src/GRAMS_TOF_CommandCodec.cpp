#include "GRAMS_TOF_CommandCodec.h"
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
        std::cerr << "[Codec] Invalid header.\n";
        return false;
    }

    uint16_t code = read16(ptr + 4);
    uint16_t argc = read16(ptr + 6);

    size_t expectedSize = 12 + argc * 4;
    if (data.size() < expectedSize) {
        std::cerr << "[Codec] Packet too short for argc = " << argc << ".\n";
        return false;
    }

    std::vector<int32_t> argv;
    const uint8_t* argv_ptr = ptr + 8;
    for (size_t i = 0; i < argc; ++i) {
        argv.push_back(read32(argv_ptr + i * 4));
    }

    uint16_t footer1 = read16(argv_ptr + argc * 4 + 2);
    uint16_t footer2 = read16(argv_ptr + argc * 4 + 4);
    if (footer1 != 0xC5A4 || footer2 != 0xD279) {
        std::cerr << "[Codec] Invalid footer.\n";
        return false;
    }

    std::cout << "[Parser] Parsed code = 0x" << std::hex << code << ", argc = " << argc << std::dec << "\n";
    outPacket.code = static_cast<TOFCommandCode>(code);
    outPacket.argv = std::move(argv);
    return true;
}

std::vector<uint8_t> GRAMS_TOF_CommandCodec::serialize(const Packet& pkt) {
    std::vector<uint8_t> buf;

    write16(buf, 0xEB90); // Header 1
    write16(buf, 0x5B6A); // Header 2
    write16(buf, static_cast<uint16_t>(pkt.code));
    write16(buf, static_cast<uint16_t>(pkt.argv.size()));

    for (int32_t arg : pkt.argv) {
        write32(buf, arg);
    }

    write16(buf, 0x0000); // CRC (dummy for now)
    write16(buf, 0xC5A4); // Footer 1
    write16(buf, 0xD279); // Footer 2

    return buf;
}

