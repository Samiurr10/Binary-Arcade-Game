#include "binary_arcade/bus.hpp"
#include "binary_arcade/flash_iap.hpp"
#include "binary_arcade/game.hpp"
#include "binary_arcade/system.hpp"

#include <chrono>
#include <iostream>

int main() {
    using namespace binary_arcade;
    using clock = std::chrono::steady_clock;

    constexpr std::size_t iterations = 250000;
    GameEngine game(0x1234);
    RecordingI2cBus i2c;
    RecordingUartSink uart;
    I2cDisplay display(i2c);
    UartTelemetry telemetry(uart);
    FlashIap flash(8, 240);
    ArcadeSystem system(game, display, telemetry, flash);

    const auto start = clock::now();
    for (std::size_t i = 0; i < iterations; ++i) {
        if (game.state() == GameState::game_over) {
            game.reset(static_cast<std::uint32_t>(i + 1));
        }
        const auto input = i % 13 == 0 ? Input::fire : (i % 3 == 0 ? Input::left : Input::right);
        const auto result = system.tick(input);
        (void)result;
    }
    const auto elapsed = std::chrono::duration<double, std::micro>(clock::now() - start);

    const double us_per_tick = elapsed.count() / static_cast<double>(iterations);
    const double ticks_per_second = 1000000.0 / us_per_tick;
    std::cout << "iterations=" << iterations << '\n';
    std::cout << "total_us=" << elapsed.count() << '\n';
    std::cout << "us_per_tick=" << us_per_tick << '\n';
    std::cout << "ticks_per_second=" << ticks_per_second << '\n';
    std::cout << "i2c_frames=" << i2c.writes.size() << '\n';
    std::cout << "uart_bytes=" << uart.bytes.size() << '\n';
    std::cout << "flash_writes=" << flash.writes() << '\n';
    return 0;
}
