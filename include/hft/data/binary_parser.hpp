#pragma once

#include <cstddef>
#include <span>
#include <vector>
#include <string_view>

#include "hft/data/binary_format.hpp"

namespace hft::data {

enum class BinaryHeaderError {
    None,                   // 0
    InsufficientData,       // 1
    InvalidMagic,           // 2
    UnsupportedVersion,     // 3
    InvalidHeaderSize,      // 4
    InvalidRecordSize,      // 5
    UnsupportedFlags,       // 6
    InvalidPriceScale,      // 7
    InvalidReservedField,   // 8
    InvalidSymbolTableSize, // 9
    FileSizeMismatch        // 10
};

[[nodiscard]]
bool decodeBinaryHeader(
    std::span<const std::byte> file,
    BinaryHeader& output,
    BinaryHeaderError& error
) noexcept;

[[nodiscard]]
bool decodeSymbolTable(
    std::span<const std::byte> file,
    std::vector<std::string_view>& output,
    BinaryHeaderError& error
) noexcept;

} // namespace hft::data