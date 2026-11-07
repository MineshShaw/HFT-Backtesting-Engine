
#include "hft/data/binary_parser.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <span>
#include <string>

namespace hft::data {

namespace {

std::vector<std::string> companyCodes = {
        "AAPL", "MSFT", "GOOGL", "AMZN", "META", "TSLA", "NVDA", "BRK.B", "V", "JPM",
        "JNJ",  "WMT",  "UNH",   "MA",   "PG",   "HD",   "CVX",  "LLY",   "ABBV", "BAC",
        "PEP",  "KO",   "MRK",   "COST", "TMO",  "AVGO", "MCD",  "CRM",   "CSCO", "ABT",
        "DHR",  "ACN",  "LIN",   "PFE",  "DIS",  "TXN",  "NKE",  "PM",    "NEE",  "VZ",
        "ADBE", "CMCSA","AMD",   "QCOM", "INTC", "WFC",  "HON",  "COP",   "UPS",  "BA"
    };

void writeU16LE(
    std::vector<std::byte>& buffer,
    std::size_t offset,
    std::uint16_t value
) {
    for (std::size_t i = 0; i < 2; ++i) {
        buffer[offset + i] = static_cast<std::byte>(
            (value >> (i * 8)) & 0xFF
        );
    }
}

void writeU32LE(
    std::vector<std::byte>& buffer,
    std::size_t offset,
    std::uint32_t value
) {
    for (std::size_t i = 0; i < 4; ++i) {
        buffer[offset + i] = static_cast<std::byte>(
            (value >> (i * 8)) & 0xFF
        );
    }
}

void writeU64LE(
    std::vector<std::byte>& buffer,
    std::size_t offset,
    std::uint64_t value
) {
    for (std::size_t i = 0; i < 8; ++i) {
        buffer[offset + i] = static_cast<std::byte>(
            (value >> (i * 8)) & 0xFF
        );
    }
}

std::vector<std::byte> makeEmptyBinaryFile() {
    std::vector<std::byte> file(kBinaryFixedHeaderSize);

    for (std::size_t i = 0; i < kBinaryMagic.size(); ++i) {
        file[i] = static_cast<std::byte>(kBinaryMagic[i]);
    }

    writeU16LE(file, 8, 1);
    writeU16LE(file, 10, 0);

    writeU32LE(file, 12, 48);
    writeU32LE(file, 16, 24);
    writeU32LE(file, 20, 0);

    writeU64LE(file, 24, 0);

    writeU32LE(file, 32, 0);
    writeU32LE(file, 36, 0);

    writeU32LE(file, 40, 10000);
    writeU32LE(file, 44, 0);

    return file;
}

std::vector<std::byte> makeBinaryFileWithOneTick() {
    constexpr std::size_t symbolTableSize = 6;

    std::vector<std::byte> file(
        48 + symbolTableSize + 24
    );

    for (std::size_t i = 0; i < kBinaryMagic.size(); ++i) {
        file[i] = static_cast<std::byte>(kBinaryMagic[i]);
    }

    writeU16LE(file, 8, 1);
    writeU16LE(file, 10, 0);

    writeU32LE(file, 12, 54);
    writeU32LE(file, 16, 24);
    writeU32LE(file, 20, 0);

    writeU64LE(file, 24, 1);

    writeU32LE(file, 32, 1);
    writeU32LE(file, 36, 6);

    writeU32LE(file, 40, 10000);
    writeU32LE(file, 44, 0);

    // Symbol table begins at offset 48.
    writeU16LE(file, 48, 4);

    file[50] = std::byte{'I'};
    file[51] = std::byte{'N'};
    file[52] = std::byte{'F'};
    file[53] = std::byte{'Y'};

    return file;
}

std::vector<std::byte> makeBinaryFileWithFiftyTicks() {
    constexpr std::size_t symbolTableSize = 260;

    std::vector<std::byte> file(
        48 + symbolTableSize + 24 * 2
    );

    for (std::size_t i = 0; i < kBinaryMagic.size(); ++i) {
        file[i] = static_cast<std::byte>(kBinaryMagic[i]);
    }

    writeU16LE(file, 8, 1);
    writeU16LE(file, 10, 0);

    writeU32LE(file, 12, 48 + symbolTableSize);
    writeU32LE(file, 16, 24);
    writeU32LE(file, 20, 0);

    writeU64LE(file, 24, 2);

    writeU32LE(file, 32, 50);
    writeU32LE(file, 36, symbolTableSize);

    writeU32LE(file, 40, 10000);
    writeU32LE(file, 44, 0);

    std::size_t offset = 48;

    for (std::size_t i = 0; i < companyCodes.size(); ++i) {
        const std::string& code = companyCodes[i];

        writeU16LE(file, offset, static_cast<std::uint16_t>(code.size()));
        offset += 2;

        for (std::size_t j = 0; j < code.size(); ++j) {
            file[offset++] = static_cast<std::byte>(code[j]);
        }
    }

    return file;

}

}

TEST(BinaryParserTest, DecodesValidEmptyFile) {
    auto file = makeEmptyBinaryFile();

    BinaryHeader header{};
    BinaryHeaderError error{};

    const bool result = decodeBinaryHeader(
        std::span<const std::byte>(file),
        header,
        error
    );

    ASSERT_TRUE(result);
    EXPECT_EQ(error, BinaryHeaderError::None);

    EXPECT_EQ(header.version_major, 1);
    EXPECT_EQ(header.version_minor, 0);
    EXPECT_EQ(header.record_count, 0);
    EXPECT_EQ(header.price_scale, 10000);
}

TEST(BinaryParserTest, DecodesValidFileWithOneTick) {
    auto file = makeBinaryFileWithOneTick();

    BinaryHeader header{};
    BinaryHeaderError error{};

    ASSERT_TRUE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(header.header_size, 54);
    EXPECT_EQ(header.record_count, 1);
    EXPECT_EQ(header.symbol_count, 1);
    EXPECT_EQ(header.symbol_table_size, 6);

    EXPECT_EQ(file.size(), 78);
}

TEST(BinaryParserTest, DecodesValidFileWithFiftyTicks) {
    auto file = makeBinaryFileWithFiftyTicks();

    BinaryHeader header{};
    BinaryHeaderError error{};

    ASSERT_TRUE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(header.header_size, 308);
    EXPECT_EQ(header.record_count, 2);
    EXPECT_EQ(header.symbol_count, 50);
    EXPECT_EQ(header.symbol_table_size, 260);

    EXPECT_EQ(file.size(), 48 + 260 + 24 * 2);
}

TEST(BinaryParserTest, RejectsInsufficientData) {
    auto file = makeEmptyBinaryFile();

    file.resize(10);

    BinaryHeader header{};
    BinaryHeaderError error{};

    EXPECT_FALSE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(error, BinaryHeaderError::InsufficientData);
}

TEST(BinaryParserTest, RejectsInvalidMagic) {
    auto file = makeEmptyBinaryFile();

    file[0] = std::byte{'X'};

    BinaryHeader header{};
    BinaryHeaderError error{};

    EXPECT_FALSE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(error, BinaryHeaderError::InvalidMagic);
}

TEST(BinaryParserTest, RejectsUnsupportedVersionMajor) {
    auto file = makeEmptyBinaryFile();

    writeU16LE(file, 8, 2);

    BinaryHeader header{};
    BinaryHeaderError error{};

    EXPECT_FALSE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(error, BinaryHeaderError::UnsupportedVersion);
}

TEST(BinaryParserTest, RejectsUnsupportedVersionMinor) {
    auto file = makeEmptyBinaryFile();

    writeU16LE(file, 10, 1);

    BinaryHeader header{};
    BinaryHeaderError error{};

    EXPECT_FALSE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(error, BinaryHeaderError::UnsupportedVersion);
}

TEST(BinaryParserTest, RejectsTruncatedFile) {
    auto file = makeBinaryFileWithOneTick();

    file.pop_back();

    BinaryHeader header{};
    BinaryHeaderError error{};

    EXPECT_FALSE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(error, BinaryHeaderError::FileSizeMismatch);
}

TEST(BinaryParserTest, RejectsTrailingBytes) {
    auto file = makeEmptyBinaryFile();

    file.push_back(std::byte{0xFF});

    BinaryHeader header{};
    BinaryHeaderError error{};

    EXPECT_FALSE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(error, BinaryHeaderError::FileSizeMismatch);
}

TEST(BinaryParserTest, RejectsInvalidHeaderSize) {
    auto file = makeEmptyBinaryFile();

    writeU32LE(file, 12, 100);

    BinaryHeader header{};
    BinaryHeaderError error{};

    EXPECT_FALSE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(error, BinaryHeaderError::InvalidHeaderSize);
}

TEST(BinaryParserTest, RejectsInvalidRecordSize) {
    auto file = makeEmptyBinaryFile();

    writeU32LE(file, 16, 100);

    BinaryHeader header{};
    BinaryHeaderError error{};

    EXPECT_FALSE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(error, BinaryHeaderError::InvalidRecordSize);
}

TEST(BinaryParserTest, RejectsUnsupportedFlags) {
    auto file = makeEmptyBinaryFile();

    writeU32LE(file, 20, 1);

    BinaryHeader header{};
    BinaryHeaderError error{};

    EXPECT_FALSE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(error, BinaryHeaderError::UnsupportedFlags);
}

TEST(BinaryParserTest, RejectsInvalidPriceScale) {
    auto file = makeEmptyBinaryFile();

    writeU32LE(file, 40, 0);

    BinaryHeader header{};
    BinaryHeaderError error{};

    EXPECT_FALSE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(error, BinaryHeaderError::InvalidPriceScale);
}

TEST(BinaryParserTest, RejectsInvalidReservedField) {
    auto file = makeEmptyBinaryFile();

    writeU32LE(file, 44, 1);

    BinaryHeader header{};
    BinaryHeaderError error{};

    EXPECT_FALSE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(error, BinaryHeaderError::InvalidReservedField);
}

TEST(BinaryParserTest, RejectsInvalidSymbolTableSize) {
    auto file = makeBinaryFileWithOneTick();

    writeU32LE(file, 36, 1);

    BinaryHeader header{};
    BinaryHeaderError error{};

    EXPECT_FALSE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(error, BinaryHeaderError::InvalidSymbolTableSize);
}

TEST(BinaryParserTest, RejectRecordOverflow) {
    auto file = makeEmptyBinaryFile();

    writeU64LE(file, 24, UINT64_MAX);

    BinaryHeader header{};
    BinaryHeaderError error{};

    EXPECT_FALSE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(error, BinaryHeaderError::FileSizeMismatch);
}

TEST(BinaryParserTest, RejectFileOverflow) {
    auto file = makeEmptyBinaryFile();

    writeU64LE(file, 24, UINT64_MAX / 24);

    BinaryHeader header{};
    BinaryHeaderError error{};

    EXPECT_FALSE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(error, BinaryHeaderError::FileSizeMismatch);
}

TEST(BinaryParserTest, LeavesOutputUnchangedOnFailure) {
    auto file = makeBinaryFileWithOneTick();

    BinaryHeader header{};
    BinaryHeaderError error{};

    ASSERT_TRUE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(header.version_major, 1);
    EXPECT_EQ(header.version_minor, 0);
    EXPECT_EQ(header.header_size, 54);
    EXPECT_EQ(header.record_size, 24);
    EXPECT_EQ(header.flags, 0);
    EXPECT_EQ(header.record_count, 1);
    EXPECT_EQ(header.symbol_count, 1);
    EXPECT_EQ(header.symbol_table_size, 6);
    EXPECT_EQ(header.price_scale, 10000);
    EXPECT_EQ(header.reserved, 0);

    EXPECT_EQ(file.size(), 78);

    file = makeBinaryFileWithOneTick();
    file.pop_back();

    EXPECT_FALSE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(header.version_major, 1);
    EXPECT_EQ(header.version_minor, 0);
    EXPECT_EQ(header.header_size, 54);
    EXPECT_EQ(header.record_size, 24);
    EXPECT_EQ(header.flags, 0);
    EXPECT_EQ(header.record_count, 1);
    EXPECT_EQ(header.symbol_count, 1);
    EXPECT_EQ(header.symbol_table_size, 6);
    EXPECT_EQ(header.price_scale, 10000);
    EXPECT_EQ(header.reserved, 0);

    EXPECT_EQ(file.size(), 77);
}

TEST(BinaryParserTest, DecodesSymbolTable) {
    auto file = makeBinaryFileWithOneTick();

    BinaryHeader header{};
    BinaryHeaderError error{};

    ASSERT_TRUE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(header.symbol_count, 1);
    EXPECT_EQ(header.symbol_table_size, 6);

    std::vector<std::string_view> symbolTable(header.symbol_count);
    ASSERT_TRUE(decodeSymbolTable(file, symbolTable, error));

    EXPECT_EQ(symbolTable.size(), 1);
    EXPECT_EQ(symbolTable[0], "INFY");
}

TEST(BinaryParserTest, DecodesSymbolTableWithFiftySymbols) {
    auto file = makeBinaryFileWithFiftyTicks();

    BinaryHeader header{};
    BinaryHeaderError error{};

    ASSERT_TRUE(decodeBinaryHeader(file, header, error));

    EXPECT_EQ(header.symbol_count, 50);
    EXPECT_EQ(header.symbol_table_size, 260);

    std::vector<std::string_view> symbolTable(header.symbol_count);
    ASSERT_TRUE(decodeSymbolTable(file, symbolTable, error));

    EXPECT_EQ(symbolTable.size(), 50);
    for (std::size_t i = 0; i < companyCodes.size(); ++i) {
        EXPECT_EQ(symbolTable[i], companyCodes[i]);
    }
}

} // namespace hft::data