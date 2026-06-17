#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace binary_arcade {

struct SaveRecord {
    std::uint32_t sequence;
    std::uint32_t high_score;
    std::uint32_t games_played;
};

class FlashIap {
  public:
    static constexpr std::uint8_t erased_value = 0xFF;

    FlashIap(std::size_t page_count = 4, std::size_t page_size = 256);

    [[nodiscard]] bool save(const SaveRecord& record);
    [[nodiscard]] std::optional<SaveRecord> load_latest() const;
    void erase_all();

    [[nodiscard]] std::size_t page_count() const;
    [[nodiscard]] std::size_t page_size() const;
    [[nodiscard]] std::size_t writes() const;
    [[nodiscard]] std::vector<std::uint32_t> wear_counts() const;
    [[nodiscard]] std::span<const std::uint8_t> raw() const;

  private:
    [[nodiscard]] std::optional<std::size_t> find_free_slot() const;
    void erase_page(std::size_t page);

    std::size_t page_count_;
    std::size_t page_size_;
    std::vector<std::uint8_t> storage_;
    std::vector<std::uint32_t> wear_;
    std::size_t writes_{0};
};

}  // namespace binary_arcade
