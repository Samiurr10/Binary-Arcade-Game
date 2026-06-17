#pragma once

#include "binary_arcade/crc.hpp"
#include "binary_arcade/framebuffer.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace binary_arcade {

class I2cBus {
  public:
    virtual ~I2cBus() = default;
    virtual bool write(std::uint8_t address, std::span<const std::uint8_t> bytes) = 0;
};

class UartSink {
  public:
    virtual ~UartSink() = default;
    virtual void write(std::span<const std::uint8_t> bytes) = 0;
};

class SpiDevice {
  public:
    virtual ~SpiDevice() = default;
    virtual std::vector<std::uint8_t> transfer(std::span<const std::uint8_t> tx) = 0;
};

class I2cDisplay {
  public:
    explicit I2cDisplay(I2cBus& bus, std::uint8_t address = 0x3C);
    [[nodiscard]] bool flush(const Framebuffer& framebuffer);

  private:
    I2cBus& bus_;
    std::uint8_t address_;
};

class UartTelemetry {
  public:
    explicit UartTelemetry(UartSink& sink);
    void emit_score(std::uint32_t score, bool game_over);

  private:
    UartSink& sink_;
};

class SpiSpriteRom {
  public:
    explicit SpiSpriteRom(SpiDevice& device);
    [[nodiscard]] std::vector<std::uint8_t> read_sprite(std::uint8_t sprite_id,
                                                        std::size_t length);

  private:
    SpiDevice& device_;
};

struct I2cWrite {
    std::uint8_t address;
    std::vector<std::uint8_t> bytes;
};

class RecordingI2cBus final : public I2cBus {
  public:
    bool write(std::uint8_t address, std::span<const std::uint8_t> bytes) override;
    std::vector<I2cWrite> writes;
    bool acknowledge = true;
};

class RecordingUartSink final : public UartSink {
  public:
    void write(std::span<const std::uint8_t> bytes) override;
    [[nodiscard]] std::string text() const;
    std::vector<std::uint8_t> bytes;
};

class MemorySpiDevice final : public SpiDevice {
  public:
    explicit MemorySpiDevice(std::vector<std::uint8_t> data);
    std::vector<std::uint8_t> transfer(std::span<const std::uint8_t> tx) override;

  private:
    std::vector<std::uint8_t> data_;
};

}  // namespace binary_arcade
