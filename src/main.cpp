#include "binary_arcade/bus.hpp"
#include "binary_arcade/flash_iap.hpp"
#include "binary_arcade/game.hpp"
#include "binary_arcade/system.hpp"

#include <iostream>
#include <vector>

int main() {
    using namespace binary_arcade;

    GameEngine game(42);
    RecordingI2cBus i2c;
    RecordingUartSink uart;
    I2cDisplay display(i2c);
    UartTelemetry telemetry(uart);
    FlashIap flash;
    ArcadeSystem system(game, display, telemetry, flash);

    for (int frame = 0; frame < 24 && game.state() == GameState::running; ++frame) {
        const Input input = frame % 5 == 0 ? Input::left : (frame % 7 == 0 ? Input::right : Input::none);
        const auto result = system.tick(input);
        (void)result;
    }

    std::cout << game.framebuffer().to_ascii();
    std::cout << "score=" << game.score() << " high_score=" << game.high_score()
              << " i2c_frames=" << i2c.writes.size()
              << " flash_writes=" << flash.writes() << '\n';
    std::cout << uart.text();
    return 0;
}
