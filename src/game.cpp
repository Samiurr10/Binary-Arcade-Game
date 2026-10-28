#include "binary_arcade/game.hpp"

#include <algorithm>

namespace binary_arcade {

GameEngine::GameEngine(std::uint32_t seed) : rng_(seed) {
    render();
}

TickResult GameEngine::tick(Input input) {
    if (state_ == GameState::game_over) {
        framebuffer_ = game_over_frame();
        return {state_, score_, false};
    }

    const bool left = has_input(input, Input::left);
    const bool right = has_input(input, Input::right);
    if (left && !right && player_x_ > 0) {
        --player_x_;
    }
    if (right && !left && player_x_ + 1 < Framebuffer::width) {
        ++player_x_;
    }
    if (has_input(input, Input::fire)) {
        obstacles_[Framebuffer::height - 2] = static_cast<std::uint16_t>(
            obstacles_[Framebuffer::height - 2] & ~(1U << player_x_));
    }

    for (std::size_t row = Framebuffer::height - 1; row > 0; --row) {
        obstacles_[row] = obstacles_[row - 1];
    }
    obstacles_[0] = 0;

    if (tick_count_ % 2 == 0) {
        spawn_row();
    }
    ++tick_count_;

    if (collides()) {
        state_ = GameState::game_over;
        const bool changed = score_ > high_score_;
        high_score_ = std::max(high_score_, score_);
        framebuffer_ = game_over_frame();
        return {state_, score_, changed};
    }

    ++score_;
    render();
    return {state_, score_, false};
}

const Framebuffer& GameEngine::framebuffer() const {
    return framebuffer_;
}

std::uint32_t GameEngine::score() const {
    return score_;
}

GameState GameEngine::state() const {
    return state_;
}

std::size_t GameEngine::player_x() const {
    return player_x_;
}

void GameEngine::reset(std::uint32_t seed) {
    rng_ = seed;
    tick_count_ = 0;
    score_ = 0;
    player_x_ = Framebuffer::width / 2;
    state_ = GameState::running;
    obstacles_.fill(0);
    render();
}

void GameEngine::set_high_score(std::uint32_t value) {
    high_score_ = value;
}

std::uint32_t GameEngine::high_score() const {
    return high_score_;
}

std::uint32_t GameEngine::next_random() {
    rng_ = rng_ * 1664525U + 1013904223U;
    return rng_;
}

void GameEngine::spawn_row() {
    const auto column = (next_random() >> 16U) % Framebuffer::width;
    const auto second = (column + 5 + ((next_random() >> 16U) % 7)) % Framebuffer::width;
    obstacles_[0] = static_cast<std::uint16_t>((1U << column) | (1U << second));
}

void GameEngine::render() {
    framebuffer_.clear();
    for (std::size_t y = 0; y < Framebuffer::height; ++y) {
        for (std::size_t x = 0; x < Framebuffer::width; ++x) {
            if ((obstacles_[y] & (1U << x)) != 0) {
                framebuffer_.set(x, y, true);
            }
        }
    }
    framebuffer_.set(player_x_, Framebuffer::height - 1, true);
}

bool GameEngine::collides() const {
    return (obstacles_[Framebuffer::height - 1] & (1U << player_x_)) != 0;
}

}  // namespace binary_arcade
