#include "binary_arcade/flash_iap.hpp"

#include "binary_arcade/crc.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>

namespace binary_arcade {
namespace {

constexpr std::uint32_t magic = 0xB1A2CAFEU;
constexpr std::size_t record_size = 24;

void put32(std::span<std::uint8_t> out, std::size_t offset, std::uint32_t value) {
    out[offset] = static_cast<std::uint8_t>(value & 0xFFU);
    out[offset + 1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    out[offset + 2] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    out[offset + 3] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
}

std::uint32_t get32(std::span<const std::uint8_t> in, std::size_t offset) {
    return static_cast<std::uint32_t>(in[offset]) |
           (static_cast<std::uint32_t>(in[offset + 1]) << 8U) |
           (static_cast<std::uint32_t>(in[offset + 2]) << 16U) |
           (static_cast<std::uint32_t>(in[offset + 3]) << 24U);
}

std::array<std::uint8_t, record_size> encode(const SaveRecord& record) {
    std::array<std::uint8_t, record_size> bytes{};
    bytes.fill(0);
    put32(bytes, 0, magic);
    put32(bytes, 4, record.sequence);
    put32(bytes, 8, record.high_score);
    put32(bytes, 12, record.games_played);
    put32(bytes, 16, 0x00010000U);
    const auto crc = crc16_ccitt(std::span<const std::uint8_t>(bytes).first(20));
    bytes[20] = static_cast<std::uint8_t>(crc & 0xFFU);
    bytes[21] = static_cast<std::uint8_t>((crc >> 8U) & 0xFFU);
    bytes[22] = 0x5A;
    bytes[23] = 0xA5;
    return bytes;
}

std::optional<SaveRecord> decode(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < record_size || get32(bytes, 0) != magic || bytes[22] != 0x5A ||
        bytes[23] != 0xA5) {
        return std::nullopt;
    }
    const auto stored = static_cast<std::uint16_t>(bytes[20] | (bytes[21] << 8U));
    const auto computed = crc16_ccitt(bytes.first(20));
    if (stored != computed) {
        return std::nullopt;
    }
    return SaveRecord{get32(bytes, 4), get32(bytes, 8), get32(bytes, 12)};
}

}  // namespace

FlashIap::FlashIap(std::size_t page_count, std::size_t page_size)
    : page_count_(page_count),
      page_size_(page_size),
      storage_(page_count * page_size, erased_value),
      wear_(page_count, 0) {
    if (page_count_ == 0 || page_size_ < record_size) {
        throw std::invalid_argument("FlashIap requires non-empty pages with room for records");
    }
}

bool FlashIap::save(const SaveRecord& record) {
    auto slot = find_free_slot();
    if (!slot.has_value()) {
        const auto wear_min = std::min_element(wear_.begin(), wear_.end());
        erase_page(static_cast<std::size_t>(std::distance(wear_.begin(), wear_min)));
        slot = find_free_slot();
    }
    if (!slot.has_value()) {
        return false;
    }

    const auto encoded = encode(record);
    std::copy(encoded.begin(), encoded.end(),
              storage_.begin() + static_cast<std::ptrdiff_t>(*slot));
    ++writes_;
    return true;
}

std::optional<SaveRecord> FlashIap::load_latest() const {
    std::optional<SaveRecord> latest;
    for (std::size_t page = 0; page < page_count_; ++page) {
        const std::size_t begin = page * page_size_;
        const std::size_t end = begin + page_size_;
        for (std::size_t offset = begin; offset + record_size <= end; offset += record_size) {
            const auto decoded = decode(std::span<const std::uint8_t>(storage_).subspan(
                offset, record_size));
            if (decoded.has_value() &&
                (!latest.has_value() || decoded->sequence > latest->sequence)) {
                latest = decoded;
            }
        }
    }
    return latest;
}

void FlashIap::erase_all() {
    for (std::size_t page = 0; page < page_count_; ++page) {
        erase_page(page);
    }
}

std::size_t FlashIap::page_count() const {
    return page_count_;
}

std::size_t FlashIap::page_size() const {
    return page_size_;
}

std::size_t FlashIap::writes() const {
    return writes_;
}

std::vector<std::uint32_t> FlashIap::wear_counts() const {
    return wear_;
}

std::span<const std::uint8_t> FlashIap::raw() const {
    return storage_;
}

std::optional<std::size_t> FlashIap::find_free_slot() const {
    for (std::size_t page = 0; page < page_count_; ++page) {
        const std::size_t begin = page * page_size_;
        const std::size_t end = begin + page_size_;
        for (std::size_t offset = begin; offset + record_size <= end; offset += record_size) {
            const auto slot =
                std::span<const std::uint8_t>(storage_).subspan(offset, record_size);
            if (std::all_of(slot.begin(), slot.end(),
                            [](std::uint8_t value) { return value == erased_value; })) {
                return offset;
            }
        }
    }
    return std::nullopt;
}

void FlashIap::erase_page(std::size_t page) {
    const std::size_t begin = page * page_size_;
    const std::size_t end = begin + page_size_;
    std::fill(storage_.begin() + static_cast<std::ptrdiff_t>(begin),
              storage_.begin() + static_cast<std::ptrdiff_t>(end), erased_value);
    ++wear_[page];
}

}  // namespace binary_arcade
