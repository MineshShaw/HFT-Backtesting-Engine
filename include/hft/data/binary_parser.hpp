#pragma once

#include <cstddef>
#include <span>

#include "hft/data/binary_format.hpp"

namespace hft::data {

enum class BinaryHeaderError {
    None,
    InsufficientData,
    InvalidMagic,
    UnsupportedVersion,
    InvalidHeaderSize,
    InvalidRecordSize,
    UnsupportedFlags,
    InvalidPriceScale,
    InvalidReservedField,
    InvalidSymbolTableSize,
    FileSizeMismatch
};

[[nodiscard]]
bool decodeBinaryHeader(
    std::span<const std::byte> file,
    BinaryHeader& output,
    BinaryHeaderError& error
) noexcept;

} // namespace hft::data