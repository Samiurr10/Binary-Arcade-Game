#include "binary_arcade/system.hpp"

namespace binary_arcade {

ArcadeSystem::ArcadeSystem(GameEngine& game, I2cDisplay& display,
                           UartTelemetry& telemetry, FlashIap& flash)
    : game_(game), display_(display), telemetry_(telemetry), flash_(flash) {
    if (const auto save = flash_.load_latest(); save.has_value()) {
        game_.set_high_score(save->high_score);
        games_played_ = save->games_played;
    }
}

TickResult ArcadeSystem::tick(Input input) {
    const auto result = game_.tick(input);
    const bool display_ok = display_.flush(game_.framebuffer());
    (void)display_ok;
    telemetry_.emit_score(result.score, result.state == GameState::game_over);

    if (result.state == GameState::game_over && result.high_score_changed) {
        ++games_played_;
        const bool saved =
            flash_.save(SaveRecord{games_played_, game_.high_score(), games_played_});
        (void)saved;
    }
    return result;
}

SaveRecord ArcadeSystem::current_save() const {
    return SaveRecord{games_played_, game_.high_score(), games_played_};
}

}  // namespace binary_arcade
