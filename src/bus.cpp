#include "binary_arcade/bus.hpp"

#include <algorithm>
#include <charconv>
#include <string>

namespace binary_arcade {

I2cDisplay::I2cDisplay(I2cBus& bus, std::uint8_t address) : bus_(bus), address_(address) {}

bool I2cDisplay::flush(const Framebuffer& framebuffer) {
    const auto payload = framebuffer.pack();
    std::vector<std::uint8_t> frame;
    frame.reserve(payload.size() + 4);
    frame.push_back(0xA5);
    frame.push_back(static_cast<std::uint8_t>(payload.size()));
    frame.insert(frame.end(), payload.begin(), payload.end());
    const auto crc = crc16_ccitt(frame);
    frame.push_back(static_cast<std::uint8_t>(crc >> 8U));
    frame.push_back(static_cast<std::uint8_t>(crc & 0xFFU));
    return bus_.write(address_, frame);
}

UartTelemetry::UartTelemetry(UartSink& sink) : sink_(sink) {}

void UartTelemetry::emit_score(std::uint32_t score, bool game_over) {
    std::string line = "score=" + std::to_string(score) +
                       ";state=" + (game_over ? "game_over" : "running");
    std::vector<std::uint8_t> bytes(line.begin(), line.end());
    const auto crc = crc16_ccitt(bytes);
    line += ";crc=" + std::to_string(crc) + "\n";
    bytes.assign(line.begin(), line.end());
    sink_.write(bytes);
}

SpiSpriteRom::SpiSpriteRom(SpiDevice& device) : device_(device) {}

std::vector<std::uint8_t> SpiSpriteRom::read_sprite(std::uint8_t sprite_id,
                                                    std::size_t length) {
    const std::vector<std::uint8_t> command = {
        0x03,
        sprite_id,
        static_cast<std::uint8_t>((length >> 8U) & 0xFFU),
        static_cast<std::uint8_t>(length & 0xFFU),
    };
    return device_.transfer(command);
}

bool RecordingI2cBus::write(std::uint8_t address, std::span<const std::uint8_t> bytes) {
    writes.push_back({address, std::vector<std::uint8_t>(bytes.begin(), bytes.end())});
    return acknowledge;
}

void RecordingUartSink::write(std::span<const std::uint8_t> data) {
    bytes.insert(bytes.end(), data.begin(), data.end());
}

std::string RecordingUartSink::text() const {
    return {bytes.begin(), bytes.end()};
}

MemorySpiDevice::MemorySpiDevice(std::vector<std::uint8_t> data) : data_(std::move(data)) {}

std::vector<std::uint8_t> MemorySpiDevice::transfer(std::span<const std::uint8_t> tx) {
    if (tx.size() != 4 || tx[0] != 0x03) {
        return {};
    }
    const std::size_t offset = static_cast<std::size_t>(tx[1]) * 16;
    const std::size_t length = (static_cast<std::size_t>(tx[2]) << 8U) | tx[3];
    if (offset >= data_.size()) {
        return {};
    }
    const auto end = std::min(data_.size(), offset + length);
    return {data_.begin() + static_cast<std::ptrdiff_t>(offset),
            data_.begin() + static_cast<std::ptrdiff_t>(end)};
}

}  // namespace binary_arcade
