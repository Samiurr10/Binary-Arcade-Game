#include "binary_arcade/framebuffer.hpp"

#include <stdexcept>

namespace binary_arcade {

void Framebuffer::clear() {
    rows_.fill(0);
}

void Framebuffer::set(std::size_t x, std::size_t y, bool on) {
    if (x >= width || y >= height) {
        throw std::out_of_range("framebuffer coordinate outside panel");
    }
    const auto mask = static_cast<std::uint16_t>(1U << x);
    if (on) {
        rows_[y] = static_cast<std::uint16_t>(rows_[y] | mask);
    } else {
        rows_[y] = static_cast<std::uint16_t>(rows_[y] & ~mask);
    }
}

bool Framebuffer::get(std::size_t x, std::size_t y) const {
    if (x >= width || y >= height) {
        throw std::out_of_range("framebuffer coordinate outside panel");
    }
    return (rows_[y] & static_cast<std::uint16_t>(1U << x)) != 0;
}

std::array<std::uint8_t, Framebuffer::packed_size> Framebuffer::pack() const {
    std::array<std::uint8_t, packed_size> packed{};
    for (std::size_t row = 0; row < height; ++row) {
        packed[row * 2] = static_cast<std::uint8_t>(rows_[row] & 0xFFU);
        packed[row * 2 + 1] = static_cast<std::uint8_t>((rows_[row] >> 8U) & 0xFFU);
    }
    return packed;
}

std::string Framebuffer::to_ascii() const {
    std::string out;
    out.reserve((width + 1) * height);
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            out.push_back(get(x, y) ? '#' : '.');
        }
        out.push_back('\n');
    }
    return out;
}

Framebuffer game_over_frame() {
    Framebuffer fb;
    const char* rows[Framebuffer::height] = {
        "####..###..#..#.",
        "#.....#..#.#..#.",
        "###...###..####.",
        "#.....#..#.#..#.",
        "####..#..#.#..#.",
        "................",
        ".###..#..#.####.",
        "#...#..##..#....",
    };

    for (std::size_t y = 0; y < Framebuffer::height; ++y) {
        for (std::size_t x = 0; x < Framebuffer::width; ++x) {
            fb.set(x, y, rows[y][x] == '#');
        }
    }
    return fb;
}

}  // namespace binary_arcade
