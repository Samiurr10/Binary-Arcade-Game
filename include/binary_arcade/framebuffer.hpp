#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace binary_arcade {

class Framebuffer {
  public:
    static constexpr std::size_t width = 16;
    static constexpr std::size_t height = 8;
    static constexpr std::size_t packed_size = height * 2;

    void clear();
    void set(std::size_t x, std::size_t y, bool on);
    [[nodiscard]] bool get(std::size_t x, std::size_t y) const;
    [[nodiscard]] std::array<std::uint8_t, packed_size> pack() const;
    [[nodiscard]] std::string to_ascii() const;

  private:
    std::array<std::uint16_t, height> rows_{};
};

Framebuffer game_over_frame();

}  // namespace binary_arcade
