#include "hft/data/binary_dataset_reader.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>

namespace hft::data {
namespace {

using RecordFields = std::tuple<
    std::int64_t,
    std::int64_t,
    std::uint16_t,
    std::uint16_t,
    std::uint32_t
>;

void writeU16LE(std::vector<std::byte>& buffer, std::size_t offset, std::uint16_t value)
{
    for (std::size_t i = 0; i < 2; ++i) {
        buffer[offset + i] = static_cast<std::byte>((value >> (i * 8)) & 0xFFU);
    }
}

void writeU32LE(std::vector<std::byte>& buffer, std::size_t offset, std::uint32_t value)
{
    for (std::size_t i = 0; i < 4; ++i) {
        buffer[offset + i] = static_cast<std::byte>((value >> (i * 8)) & 0xFFU);
    }
}

void writeU64LE(std::vector<std::byte>& buffer, std::size_t offset, std::uint64_t value)
{
    for (std::size_t i = 0; i < 8; ++i) {
        buffer[offset + i] = static_cast<std::byte>((value >> (i * 8)) & 0xFFU);
    }
}

std::vector<std::byte> makeBinaryFile(
    const std::vector<std::string>& symbols,
    const std::vector<RecordFields>& records,
    std::uint32_t price_scale = 100,
    std::uint32_t quantity_scale = 10
)
{
    std::size_t symbol_table_size = 0;
    for (const auto& symbol : symbols) {
        symbol_table_size += 2 + symbol.size();
    }

    const std::size_t header_size = kBinaryFixedHeaderSize + symbol_table_size;
    std::vector<std::byte> file(
        header_size + records.size() * kBinaryRecordSize
    );

    for (std::size_t i = 0; i < kBinaryMagic.size(); ++i) {
        file[i] = static_cast<std::byte>(kBinaryMagic[i]);
    }

    writeU16LE(file, 8, kBinaryVersionMajor);
    writeU16LE(file, 10, kBinaryVersionMinor);
    writeU32LE(file, 12, static_cast<std::uint32_t>(header_size));
    writeU32LE(file, 16, kBinaryRecordSize);
    writeU32LE(file, 20, 0);
    writeU64LE(file, 24, records.size());
    writeU32LE(file, 32, symbols.size());
    writeU32LE(file, 36, static_cast<std::uint32_t>(symbol_table_size));
    writeU32LE(file, 40, price_scale);
    writeU32LE(file, 44, quantity_scale);

    std::size_t offset = kBinaryFixedHeaderSize;
    for (const auto& symbol : symbols) {
        writeU16LE(file, offset, static_cast<std::uint16_t>(symbol.size()));
        offset += 2;
        for (const char character : symbol) {
            file[offset++] = static_cast<std::byte>(character);
        }
    }

    for (const auto& [timestamp, price, symbol_id, reserved, quantity] : records) {
        writeU64LE(file, offset, static_cast<std::uint64_t>(timestamp));
        writeU64LE(file, offset + 8, static_cast<std::uint64_t>(price));
        writeU16LE(file, offset + 16, symbol_id);
        writeU16LE(file, offset + 18, reserved);
        writeU32LE(file, offset + 20, quantity);
        offset += kBinaryRecordSize;
    }

    return file;
}

void writeFile(const std::filesystem::path& path, const std::vector<std::byte>& data)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(file.is_open());
    file.write(
        reinterpret_cast<const char*>(data.data()),
        static_cast<std::streamsize>(data.size())
    );
    ASSERT_TRUE(file.good());
}

class BinaryDatasetReaderTest : public ::testing::Test {
protected:
    std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        "hft_binary_dataset_reader_test.bin";

    void TearDown() override
    {
        std::error_code error;
        std::filesystem::remove(path, error);
    }
};

TEST_F(BinaryDatasetReaderTest, ReadsEmptyDatasetAndReportsEof)
{
    writeFile(path, makeBinaryFile({}, {}));

    BinaryDatasetReader reader(path.string());
    ASSERT_TRUE(reader.is_valid());
    EXPECT_FALSE(reader.initialization_error().has_value());

    Tick output{123, "old", 4.5, 6.7};
    EXPECT_EQ(reader.next(output), BinaryReadStatus::EndOfFile);
    EXPECT_EQ(output.timestamp, 123);
    EXPECT_EQ(output.symbol, "old");
    EXPECT_EQ(reader.stats().records_seen, 0);
    EXPECT_EQ(reader.stats().records_emitted, 0);
    EXPECT_EQ(reader.stats().records_skipped, 0);
}

TEST_F(BinaryDatasetReaderTest, ReadsRecordsResolvesSymbolsAndAppliesScales)
{
    writeFile(
        path,
        makeBinaryFile(
            {"AAPL", "ETH"},
            {
                {100, 12345, 1, 0, 250},
                {200, -500, 0, 0, 10}
            }
        )
    );

    BinaryDatasetReader reader(path.string());
    ASSERT_TRUE(reader.is_valid());

    Tick first{};
    ASSERT_EQ(reader.next(first), BinaryReadStatus::TickAvailable);
    EXPECT_EQ(first.timestamp, 100);
    EXPECT_EQ(first.symbol, "ETH");
    EXPECT_DOUBLE_EQ(first.price, 123.45);
    EXPECT_DOUBLE_EQ(first.size, 25.0);

    Tick second{};
    ASSERT_EQ(reader.next(second), BinaryReadStatus::TickAvailable);
    EXPECT_EQ(second.timestamp, 200);
    EXPECT_EQ(second.symbol, "AAPL");
    EXPECT_DOUBLE_EQ(second.price, -5.0);
    EXPECT_DOUBLE_EQ(second.size, 1.0);

    EXPECT_EQ(reader.next(second), BinaryReadStatus::EndOfFile);
    EXPECT_EQ(reader.stats().records_seen, 2);
    EXPECT_EQ(reader.stats().records_emitted, 2);
    EXPECT_EQ(reader.stats().records_skipped, 0);
}

TEST_F(BinaryDatasetReaderTest, ReportsInvalidHeaderDuringInitialization)
{
    writeFile(path, {std::byte{0x01}});

    BinaryDatasetReader reader(path.string());
    EXPECT_FALSE(reader.is_valid());
    ASSERT_TRUE(reader.initialization_error().has_value());
    EXPECT_EQ(
        reader.initialization_error()->stage,
        BinaryErrorStage::Header
    );
    ASSERT_TRUE(reader.initialization_error()->header_error.has_value());
    EXPECT_EQ(
        reader.initialization_error()->header_error.value(),
        BinaryHeaderError::InsufficientData
    );

    Tick output{7, "old", 1.0, 2.0};
    EXPECT_EQ(reader.next(output), BinaryReadStatus::Error);
    ASSERT_TRUE(reader.last_error().has_value());
    EXPECT_EQ(reader.last_error()->stage, BinaryErrorStage::Header);
    EXPECT_EQ(output.timestamp, 7);
    EXPECT_EQ(output.symbol, "old");
}

TEST_F(BinaryDatasetReaderTest, ReportsInvalidSymbolTableDuringInitialization)
{
    auto file = makeBinaryFile({"AAPL"}, {});
    writeU16LE(file, kBinaryFixedHeaderSize, 5);
    writeFile(path, file);

    BinaryDatasetReader reader(path.string());
    EXPECT_FALSE(reader.is_valid());
    ASSERT_TRUE(reader.initialization_error().has_value());
    EXPECT_EQ(
        reader.initialization_error()->stage,
        BinaryErrorStage::SymbolTable
    );
    ASSERT_TRUE(reader.initialization_error()->header_error.has_value());
    EXPECT_EQ(
        reader.initialization_error()->header_error.value(),
        BinaryHeaderError::InsufficientData
    );
}

TEST_F(BinaryDatasetReaderTest, StopsOnCorruptionByDefault)
{
    writeFile(
        path,
        makeBinaryFile(
            {"AAPL"},
            {
                {100, 100, 0, 1, 10},
                {200, 200, 0, 0, 20}
            }
        )
    );

    BinaryDatasetReader reader(path.string());
    ASSERT_TRUE(reader.is_valid());

    Tick output{7, "old", 1.0, 2.0};
    EXPECT_EQ(reader.next(output), BinaryReadStatus::Error);
    EXPECT_EQ(output.timestamp, 7);
    EXPECT_EQ(output.symbol, "old");

    ASSERT_TRUE(reader.last_corruption().has_value());
    EXPECT_EQ(reader.last_corruption()->stage, BinaryErrorStage::Record);
    EXPECT_EQ(
        reader.last_corruption()->record_error.value(),
        BinaryRecordError::InvalidReservedField
    );
    EXPECT_EQ(reader.last_corruption()->record_index, 0);
    EXPECT_EQ(
        reader.last_corruption()->byte_offset,
        kBinaryFixedHeaderSize + 6
    );
    ASSERT_TRUE(reader.last_error().has_value());
    EXPECT_EQ(reader.last_error()->record_index, 0);
    EXPECT_EQ(reader.stats().records_seen, 1);
    EXPECT_EQ(reader.stats().records_emitted, 0);
    EXPECT_EQ(reader.stats().records_skipped, 0);
}

TEST_F(BinaryDatasetReaderTest, SkipsCorruptRecordsAndRetainsLastCorruption)
{
    writeFile(
        path,
        makeBinaryFile(
            {"AAPL"},
            {
                {100, 100, 0, 1, 10},
                {200, 200, 0, 0, 20},
                {300, 300, 1, 0, 30},
                {400, 400, 0, 0, 40}
            }
        )
    );

    BinaryDatasetReader reader(
        path.string(),
        BinaryCorruptionPolicy::SkipCorruptRecords
    );
    ASSERT_TRUE(reader.is_valid());

    Tick output{};
    ASSERT_EQ(reader.next(output), BinaryReadStatus::TickAvailable);
    EXPECT_EQ(output.timestamp, 200);
    ASSERT_EQ(reader.next(output), BinaryReadStatus::TickAvailable);
    EXPECT_EQ(output.timestamp, 400);
    EXPECT_EQ(reader.next(output), BinaryReadStatus::EndOfFile);

    EXPECT_EQ(reader.stats().records_seen, 4);
    EXPECT_EQ(reader.stats().records_emitted, 2);
    EXPECT_EQ(reader.stats().records_skipped, 2);
    ASSERT_TRUE(reader.last_corruption().has_value());
    EXPECT_EQ(reader.last_corruption()->record_index, 2);
    EXPECT_EQ(
        reader.last_corruption()->record_error.value(),
        BinaryRecordError::InvalidSymbolId
    );
    EXPECT_FALSE(reader.last_error().has_value());
}

TEST_F(BinaryDatasetReaderTest, ReportsMissingFileAsInitializationError)
{
    std::error_code error;
    std::filesystem::remove(path, error);

    BinaryDatasetReader reader(path.string());
    EXPECT_FALSE(reader.is_valid());
    ASSERT_TRUE(reader.initialization_error().has_value());
    EXPECT_EQ(
        reader.initialization_error()->stage,
        BinaryErrorStage::Mapping
    );
    ASSERT_TRUE(reader.initialization_error()->mapped_file_error.has_value());
    EXPECT_EQ(
        reader.initialization_error()->mapped_file_error.value(),
        MappedFileError::OpenFailed
    );
}

} // namespace
} // namespace hft::data
