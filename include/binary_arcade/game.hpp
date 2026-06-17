#pragma once

#include "binary_arcade/framebuffer.hpp"

#include <cstdint>
#include <optional>

namespace binary_arcade {

enum class Input : std::uint8_t {
    none = 0,
    left = 1,
    right = 2,
    fire = 4,
};

constexpr Input operator|(Input lhs, Input rhs) {
    return static_cast<Input>(static_cast<std::uint8_t>(lhs) |
                              static_cast<std::uint8_t>(rhs));
}

constexpr bool has_input(Input input, Input flag) {
    return (static_cast<std::uint8_t>(input) & static_cast<std::uint8_t>(flag)) != 0;
}

enum class GameState {
    running,
    game_over,
};

struct TickResult {
    GameState state;
    std::uint32_t score;
    bool high_score_changed;
};

class GameEngine {
  public:
    explicit GameEngine(std::uint32_t seed = 0xC0FFEEU);

    [[nodiscard]] TickResult tick(Input input);
    [[nodiscard]] const Framebuffer& framebuffer() const;
    [[nodiscard]] std::uint32_t score() const;
    [[nodiscard]] GameState state() const;
    [[nodiscard]] std::size_t player_x() const;

    void reset(std::uint32_t seed = 0xC0FFEEU);
    void set_high_score(std::uint32_t value);
    [[nodiscard]] std::uint32_t high_score() const;

  private:
    std::uint32_t next_random();
    void spawn_row();
    void render();
    bool collides() const;

    Framebuffer framebuffer_{};
    std::array<std::uint16_t, Framebuffer::height> obstacles_{};
    std::uint32_t rng_;
    std::uint32_t tick_count_{0};
    std::uint32_t score_{0};
    std::uint32_t high_score_{0};
    std::size_t player_x_{Framebuffer::width / 2};
    GameState state_{GameState::running};
};

}  // namespace binary_arcade
