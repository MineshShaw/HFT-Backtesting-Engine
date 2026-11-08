#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace hft::data {

inline constexpr std::size_t kBinaryFixedHeaderSize = 48;
inline constexpr std::uint32_t kBinaryRecordSize = 24;

inline constexpr std::uint16_t kBinaryVersionMajor = 1;
inline constexpr std::uint16_t kBinaryVersionMinor = 0;

inline constexpr std::array<std::uint8_t, 8> kBinaryMagic{'H', 'F', 'T', 'B', 'D', 'A', 'T', '\0'};

struct BinaryHeader {
    std::uint16_t version_major{};
    std::uint16_t version_minor{};

    std::uint32_t header_size{};
    std::uint32_t record_size{};
    std::uint32_t flags{};

    std::uint64_t record_count{};

    std::uint32_t symbol_count{};
    std::uint32_t symbol_table_size{};

    std::uint32_t price_scale{};
    std::uint32_t quantity_scale{};
};

struct BinaryRecord {
    std::int64_t timestamp{};
    std::uint32_t quantity{};
    std::uint16_t symbol_id{};
    std::uint16_t reserved{};
    std::int64_t price{};
};

} // namespace hft::data