#pragma once

#include <kea/exceptions/exceptions.h>
#include <cstddef>

namespace kea {
namespace util {

inline uint16_t readUint16(const void* buffer, size_t length) {
    if (length < sizeof(uint16_t)) {
        kea_throw(kea::OutOfRange,
                  "Length (" << length << ") of buffer is insufficient " <<
                  "to read a uint16_t");
    }

    const uint8_t* byte_buffer = static_cast<const uint8_t*>(buffer);
    uint16_t result = (static_cast<uint16_t>(byte_buffer[0])) << 8;
    result |= static_cast<uint16_t>(byte_buffer[1]);

    return (result);
}

inline uint8_t* writeUint16(uint16_t value, void* buffer, size_t length) {
    if (length < sizeof(uint16_t)) {
        kea_throw(kea::OutOfRange,
                  "Length (" << length << ") of buffer is insufficient " <<
                  "to write a uint16_t");
    }

    uint8_t* byte_buffer = static_cast<uint8_t*>(buffer);
    byte_buffer[0] = static_cast<uint8_t>((value & 0xff00U) >> 8);
    byte_buffer[1] = static_cast<uint8_t>(value & 0x00ffU);

    return (byte_buffer + sizeof(uint16_t));
}

inline uint32_t readUint32(const uint8_t* buffer, size_t length) {
    if (length < sizeof(uint32_t)) {
        kea_throw(kea::OutOfRange,
                "Length (" << length << ") of buffer is insufficient " <<
                "to read a uint32_t");
    }

    const uint8_t* byte_buffer = static_cast<const uint8_t*>(buffer);

    uint32_t result = (static_cast<uint32_t>(byte_buffer[0])) << 24;
    result |= (static_cast<uint32_t>(byte_buffer[1])) << 16;
    result |= (static_cast<uint32_t>(byte_buffer[2])) << 8;
    result |= (static_cast<uint32_t>(byte_buffer[3]));

    return (result);
}

inline uint8_t* writeUint32(uint32_t value, uint8_t* buffer, size_t length) {
    if (length < sizeof(uint32_t)) {
        kea_throw(kea::OutOfRange,
                  "Length (" << length << ") of buffer is insufficient " <<
                  "to write a uint32_t");
    }

    uint8_t* byte_buffer = static_cast<uint8_t*>(buffer);

    byte_buffer[0] = static_cast<uint8_t>((value & 0xff000000U) >> 24);
    byte_buffer[1] = static_cast<uint8_t>((value & 0x00ff0000U) >> 16);
    byte_buffer[2] = static_cast<uint8_t>((value & 0x0000ff00U) >>  8);
    byte_buffer[3] = static_cast<uint8_t>((value & 0x000000ffU));

    return (byte_buffer + sizeof(uint32_t));
}

};
};
