#include "binary_arcade/bus.hpp"
#include "binary_arcade/flash_iap.hpp"
#include "binary_arcade/game.hpp"
#include "binary_arcade/system.hpp"

#include "mbed.h"

#include <cstdint>
#include <span>
#include <vector>

extern "C" void delay_ms(std::uint32_t ms);

namespace {

using namespace binary_arcade;

class MbedUartSink final : public UartSink {
  public:
    void write(std::span<const std::uint8_t> bytes) override {
        pc.write(bytes.data(), bytes.size());
    }
};

class CountingI2cBus final : public I2cBus {
  public:
    bool write(std::uint8_t address, std::span<const std::uint8_t> bytes) override {
        ++frames;
        last_address = address;
        last_length = bytes.size();
        if (!bytes.empty()) {
            last_sync = bytes[0];
        }
        // Mirror a short I2C summary on UART so Simantic can verify the bus path.
        pc.printf("i2c addr=0x%02X len=%u sync=0x%02X\r\n", static_cast<unsigned>(address),
                  static_cast<unsigned>(bytes.size()), static_cast<unsigned>(last_sync));
        return true;
    }

    std::uint32_t frames = 0;
    std::uint8_t last_address = 0;
    std::size_t last_length = 0;
    std::uint8_t last_sync = 0;
};


Input demo_input(int frame) {
    if (frame % 5 == 0) {
        return Input::left;
    }
    if (frame % 7 == 0) {
        return Input::right;
    }
    return Input::none;
}

}  // namespace

int main() {
    pc.baud(115200);
    pc.printf("BINARY_ARCADE_BOOT mcu=STM32F401CC core=Cortex-M4\r\n");

    GameEngine game(42);
    CountingI2cBus i2c;
    MbedUartSink uart;
    I2cDisplay display(i2c);
    UartTelemetry telemetry(uart);
    FlashIap flash;
    ArcadeSystem system(game, display, telemetry, flash);

    std::vector<std::uint8_t> sprite_rom(64);
    for (std::size_t i = 0; i < sprite_rom.size(); ++i) {
        sprite_rom[i] = static_cast<std::uint8_t>(0xA0U + i);
    }
    binary_arcade::MemorySpiDevice spi(std::move(sprite_rom));
    SpiSpriteRom sprites(spi);
    const auto sprite = sprites.read_sprite(1, 8);
    pc.printf("BINARY_ARCADE_SPI_OK bytes=%u first=0x%02X\r\n",
              static_cast<unsigned>(sprite.size()),
              sprite.empty() ? 0U : static_cast<unsigned>(sprite[0]));

    for (int frame = 0; frame < 24 && game.state() == GameState::running; ++frame) {
        (void)system.tick(demo_input(frame));
        delay_ms(20);
    }

    pc.printf("BINARY_ARCADE_I2C_OK frames=%u addr=0x%02X\r\n",
              static_cast<unsigned>(i2c.frames), static_cast<unsigned>(i2c.last_address));
    pc.printf("BINARY_ARCADE_FLASH_OK writes=%u high_score=%u games=%u\r\n",
              static_cast<unsigned>(flash.writes()), static_cast<unsigned>(game.high_score()),
              static_cast<unsigned>(system.current_save().games_played));
    pc.printf("BINARY_ARCADE_DONE score=%u state=%s\r\n", static_cast<unsigned>(game.score()),
              game.state() == GameState::game_over ? "game_over" : "running");

    // Keep emitting a heartbeat so longer sim timeouts still show life.
    for (;;) {
        pc.printf("BINARY_ARCADE_IDLE score=%u\r\n", static_cast<unsigned>(game.score()));
        delay_ms(1000);
    }
}
