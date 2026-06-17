#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace binary_arcade {

constexpr std::uint16_t crc16_ccitt(std::span<const std::uint8_t> bytes,
                                    std::uint16_t seed = 0xFFFF) {
    std::uint16_t crc = seed;
    for (std::uint8_t byte : bytes) {
        crc ^= static_cast<std::uint16_t>(byte) << 8;
        for (std::size_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x8000U) != 0U
                      ? static_cast<std::uint16_t>((crc << 1U) ^ 0x1021U)
                      : static_cast<std::uint16_t>(crc << 1U);
        }
    }
    return crc;
}

}  // namespace binary_arcade
