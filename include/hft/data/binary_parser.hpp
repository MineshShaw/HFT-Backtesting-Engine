#pragma once

#include <cstddef>
#include <span>
#include <vector>
#include <string_view>

#include "hft/data/binary_format.hpp"

// Uncomment to debug error messages in the binary parser.
// #include <ostream>
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
    InvalidQuantityScale,   // 8
    InvalidSymbolTableSize, // 9
    FileSizeMismatch,       // 10
    InvalidSymbolCount       // 11
};

// Uncomment to debug error messages in the binary parser.
// inline std::ostream& operator<<(std::ostream& os, BinaryHeaderError error) {
//     switch (error) {
//         case BinaryHeaderError::None:                   return os << "None";
//         case BinaryHeaderError::InsufficientData:       return os << "InsufficientData";
//         case BinaryHeaderError::InvalidMagic:           return os << "InvalidMagic";
//         case BinaryHeaderError::UnsupportedVersion:     return os << "UnsupportedVersion";
//         case BinaryHeaderError::InvalidHeaderSize:      return os << "InvalidHeaderSize";
//         case BinaryHeaderError::InvalidRecordSize:      return os << "InvalidRecordSize";
//         case BinaryHeaderError::UnsupportedFlags:       return os << "UnsupportedFlags";
//         case BinaryHeaderError::InvalidPriceScale:      return os << "InvalidPriceScale";
//         case BinaryHeaderError::InvalidQuantityScale:   return os << "InvalidQuantityScale";
//         case BinaryHeaderError::InvalidSymbolTableSize: return os << "InvalidSymbolTableSize";
//         case BinaryHeaderError::FileSizeMismatch:       return os << "FileSizeMismatch";
//         default: return os << "Unknown Error (" << static_cast<int>(error) << ")";
//     }
// }

enum class BinaryRecordError {
    None,                   // 0
    InvalidRecordSize,      // 1
    InvalidSymbolId,        // 2
    InvalidReservedField    // 3
};

// Uncomment to debug error messages in the binary parser.
// inline std::ostream& operator<<(std::ostream& os, BinaryRecordError error) {
//     switch (error) {
//         case BinaryRecordError::None:                return os << "None";
//         case BinaryRecordError::InvalidRecordSize:   return os << "InvalidRecordSize";
//         case BinaryRecordError::InvalidSymbolId:     return os << "InvalidSymbolId";
//         case BinaryRecordError::InvalidReservedField: return os << "InvalidReservedField";
//         default  : return os << "Unknown Error (" << static_cast<int>(error) << ")";
//     }
// }

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
);


[[nodiscard]]
bool decodeBinaryRecord(
    std::span<const std::byte> record,
    const BinaryHeader& header,
    BinaryRecord& output,
    BinaryRecordError& error
) noexcept;

} // namespace hft::data