#pragma once

#include "binary_arcade/bus.hpp"
#include "binary_arcade/flash_iap.hpp"
#include "binary_arcade/game.hpp"

namespace binary_arcade {

class ArcadeSystem {
  public:
    ArcadeSystem(GameEngine& game, I2cDisplay& display, UartTelemetry& telemetry,
                 FlashIap& flash);

    [[nodiscard]] TickResult tick(Input input);
    [[nodiscard]] SaveRecord current_save() const;

  private:
    GameEngine& game_;
    I2cDisplay& display_;
    UartTelemetry& telemetry_;
    FlashIap& flash_;
    std::uint32_t games_played_{0};
};

}  // namespace binary_arcade
