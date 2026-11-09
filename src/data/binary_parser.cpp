#include "hft/data/binary_parser.hpp"

#include <cstdint>
#include <cstddef>
#include <bit>
#include <limits>

namespace hft::data {

namespace {

std::uint16_t readU16LE(
    std::span<const std::byte> data,
    std::size_t offset
) noexcept {
    const auto b0 = std::to_integer<std::uint8_t>(data[offset]);
    const auto b1 = std::to_integer<std::uint8_t>(data[offset + 1]);

    return static_cast<std::uint16_t>(
        b0 | (static_cast<std::uint16_t>(b1) << 8)
    );
}

std::uint32_t readU32LE(
    std::span<const std::byte> data,
    std::size_t offset
) noexcept {
    std::uint32_t value{};

    for (std::size_t i = 0; i < 4; ++i) {
        value |=
            static_cast<std::uint32_t>(
                std::to_integer<std::uint8_t>(data[offset + i])
            ) << (i * 8);
    }

    return value;
}

std::uint64_t readU64LE(
    std::span<const std::byte> data,
    std::size_t offset
) noexcept {
    std::uint64_t value{};

    for (std::size_t i = 0; i < 8; ++i) {
        value |=
            static_cast<std::uint64_t>(
                std::to_integer<std::uint8_t>(data[offset + i])
            ) << (i * 8);
    }

    return value;
}

} // namespace

bool decodeBinaryHeader(
    std::span<const std::byte> file,
    BinaryHeader& output,
    BinaryHeaderError& error
) noexcept {
    BinaryHeader header{};
    error = BinaryHeaderError::None;

    if (file.size() < kBinaryFixedHeaderSize) {
        error = BinaryHeaderError::InsufficientData;
        return false;
    }

    for (std::size_t i = 0; i < kBinaryMagic.size(); ++i) {
        if (file[i] != std::byte{kBinaryMagic[i]}) {
            error = BinaryHeaderError::InvalidMagic;
            return false;
        }
    }

    header.version_major = readU16LE(file, 8);
    header.version_minor = readU16LE(file, 10);

    if (header.version_major != kBinaryVersionMajor ||
        header.version_minor != kBinaryVersionMinor) {
        error = BinaryHeaderError::UnsupportedVersion;
        return false;
    }

    header.header_size = readU32LE(file, 12);
    header.record_size = readU32LE(file, 16);
    header.flags = readU32LE(file, 20);
    header.record_count = readU64LE(file, 24);
    header.symbol_count = readU32LE(file, 32);
    header.symbol_table_size = readU32LE(file, 36);
    header.price_scale = readU32LE(file, 40);
    header.quantity_scale = readU32LE(file, 44);

    if (header.symbol_count > std::numeric_limits<std::uint16_t>::max() + 1U) {
        error = BinaryHeaderError::InvalidSymbolCount;
        return false;
    }

    if (header.symbol_table_size >
        std::numeric_limits<std::uint32_t>::max() - kBinaryFixedHeaderSize) {
        error = BinaryHeaderError::InvalidHeaderSize;
        return false;
    }

    const std::uint64_t minimumSymbolTableSize =
    static_cast<std::uint64_t>(header.symbol_count) * 2;

    if (header.symbol_table_size < minimumSymbolTableSize) {
        error = BinaryHeaderError::InvalidSymbolTableSize;
        return false;
    }

    if (header.header_size !=
        kBinaryFixedHeaderSize + header.symbol_table_size) {
        error = BinaryHeaderError::InvalidHeaderSize;
        return false;
    }

    if (header.record_size != kBinaryRecordSize) {
        error = BinaryHeaderError::InvalidRecordSize;
        return false;
    }

    if (header.flags != 0) {
        error = BinaryHeaderError::UnsupportedFlags;
        return false;
    }

    if (header.price_scale == 0) {
        error = BinaryHeaderError::InvalidPriceScale;
        return false;
    }

    if (header.quantity_scale == 0) {
        error = BinaryHeaderError::InvalidQuantityScale;
        return false;
    }

    const std::uint64_t maxValue =
    std::numeric_limits<std::uint64_t>::max();

    if (header.record_count >
        (maxValue - header.header_size) / header.record_size) {
        error = BinaryHeaderError::FileSizeMismatch;
        return false;
    }

    const std::uint64_t expectedFileSize =
        static_cast<std::uint64_t>(header.header_size) +
        header.record_count * header.record_size;

    if (expectedFileSize >
        std::numeric_limits<std::size_t>::max()) {
        error = BinaryHeaderError::FileSizeMismatch;
        return false;
    }

    if (expectedFileSize != file.size()) {
        error = BinaryHeaderError::FileSizeMismatch;
        return false;
    }

    output = header;

    return true;
}

bool decodeSymbolTable(
    std::span<const std::byte> file,
    std::vector<std::string_view>& output,
    BinaryHeaderError& error)
{
    BinaryHeader header{};
    if (!decodeBinaryHeader(file, header, error)) {
        return false;
    }

    const std::size_t symbolTableStart = kBinaryFixedHeaderSize;
    const std::size_t symbolTableSize =
        static_cast<std::size_t>(header.symbol_table_size);
    if (symbolTableSize > std::numeric_limits<std::size_t>::max() -
                              symbolTableStart) {
        error = BinaryHeaderError::InvalidHeaderSize;
        return false;
    }

    const std::size_t symbolTableEnd = symbolTableStart + symbolTableSize;

    std::vector<std::string_view> decoded;
    decoded.reserve(header.symbol_count);
    std::size_t offset = symbolTableStart;

    for (std::size_t i = 0; i < header.symbol_count; ++i) {
        if (symbolTableEnd - offset < sizeof(std::uint16_t)) {
            error = BinaryHeaderError::InsufficientData;
            return false;
        }

        const std::uint16_t symbolLength = readU16LE(file, offset);
        offset += 2;

        if (symbolTableEnd - offset < symbolLength) {
            error = BinaryHeaderError::InsufficientData;
            return false;
        }

        decoded.emplace_back(
            reinterpret_cast<const char*>(file.data() + offset),
            symbolLength
        );

        offset += symbolLength;
    }

    if (offset != symbolTableEnd) {
        error = BinaryHeaderError::InvalidSymbolTableSize;
        return false;
    }

    output.swap(decoded);
    return true;
}

bool decodeBinaryRecord(
    std::span<const std::byte> record,
    const BinaryHeader& header,
    BinaryRecord& output,
    BinaryRecordError& error
) noexcept {
    error = BinaryRecordError::None;

    BinaryRecord tempRecord{};

    if (record.size() != kBinaryRecordSize) {
        error = BinaryRecordError::InvalidRecordSize;
        return false;
    }

    tempRecord.timestamp = std::bit_cast<std::int64_t>(readU64LE(record, 0));
    tempRecord.price = std::bit_cast<std::int64_t>(readU64LE(record, 8));
    tempRecord.symbol_id = readU16LE(record, 16);
    tempRecord.reserved = readU16LE(record, 18);
    tempRecord.quantity = readU32LE(record, 20);

    if (tempRecord.symbol_id >= header.symbol_count) {
        error = BinaryRecordError::InvalidSymbolId;
        return false;
    }

    if (tempRecord.reserved != 0) {
        error = BinaryRecordError::InvalidReservedField;
        return false;
    }

    output = tempRecord;

    return true;
}

} // namespace hft::data