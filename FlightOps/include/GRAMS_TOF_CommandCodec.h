
// -------------------------------------------------------------
// Header 1	uint16_t	 Fixed: 0xEB90
// Header 2	uint16_t	 Fixed: 0x5B6A
// Code	    uint16_t	 Command code
// Argc	    uint16_t	 Number of arguments
// Argv	    int32_t[]	 Arguments: argc × int32_t values
// CRC	    uint16_t	 CRC checksum (of all prior fields)
// Footer 1	uint16_t	 Fixed: 0xC5A4
// Footer 2	uint16_t	 Fixed: 0xD279
// -------------------------------------------------------------

#pragma once

#include "GRAMS_TOF_CommandDefs.h"
#include <vector>
#include <cstdint>

class GRAMS_TOF_CommandCodec {
public:
    struct Packet {
        TOFCommandCode code;
        std::vector<int32_t> argv;
    };

    // Parse raw bytes into Packet
    static bool parse(const std::vector<uint8_t>& data, Packet& outPacket);

    // Serialize Packet into raw bytes
    static std::vector<uint8_t> serialize(const Packet& packet);
};

