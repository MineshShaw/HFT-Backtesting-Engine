#include "hft/data/dataset_reader.hpp"

#include "gtest/gtest.h"
#include <filesystem>

using namespace hft::data;

const std::filesystem::path fixture_dir(TEST_FIXTURE_DIR);

TEST(DatasetReaderTest, ValidDataset)
{
    auto path = fixture_dir / "valid_dataset.csv";
    DatasetReader reader(path.string());
    ASSERT_TRUE(reader.is_valid());

    Tick tick;
    std::size_t count = 0;

    while (reader.next(tick)) {
        ++count;
    }

    EXPECT_EQ(count, 5); 
    EXPECT_EQ(reader.stats().lines_read, 5);
    EXPECT_EQ(reader.stats().csv_records_accepted, 5);
    EXPECT_EQ(reader.stats().csv_records_rejected, 0);
    EXPECT_EQ(reader.stats().ticks_emitted, 5);
    EXPECT_EQ(reader.stats().tick_conversion_rejected, 0);

    EXPECT_EQ(reader.stats().lines_read, reader.stats().csv_records_accepted + reader.stats().csv_records_rejected);
    EXPECT_EQ(reader.stats().csv_records_accepted, reader.stats().ticks_emitted + reader.stats().tick_conversion_rejected);
}

TEST(DatasetReaderTest, MixedValidAndInvalidRows)
{
    auto path = fixture_dir / "mixed_dataset.csv";
    DatasetReader reader(path.string());
    ASSERT_TRUE(reader.is_valid());

    Tick tick;
    std::size_t count = 0;

    while (reader.next(tick)) {
        ++count;
    }

    EXPECT_EQ(count, 5); 
    EXPECT_EQ(reader.stats().lines_read, 10); 
    EXPECT_EQ(reader.stats().csv_records_accepted, 5);
    EXPECT_EQ(reader.stats().csv_records_rejected, 5);
    EXPECT_EQ(reader.stats().ticks_emitted, 5);
    EXPECT_EQ(reader.stats().tick_conversion_rejected, 0);

    EXPECT_EQ(reader.stats().lines_read, reader.stats().csv_records_accepted + reader.stats().csv_records_rejected);
    EXPECT_EQ(reader.stats().csv_records_accepted, reader.stats().ticks_emitted + reader.stats().tick_conversion_rejected);
}

TEST(DatasetReaderTest, EmptyDataset)
{
    auto path = fixture_dir / "empty.csv";
    DatasetReader reader(path.string());
    ASSERT_TRUE(reader.is_valid());

    Tick tick;
    std::size_t count = 0;

    while (reader.next(tick)) {
        ++count;
    }

    EXPECT_EQ(count, 0);
    EXPECT_EQ(reader.stats().lines_read, 0);
    EXPECT_EQ(reader.stats().csv_records_accepted, 0);
    EXPECT_EQ(reader.stats().csv_records_rejected, 0);
    EXPECT_EQ(reader.stats().ticks_emitted, 0);
    EXPECT_EQ(reader.stats().tick_conversion_rejected, 0);

    EXPECT_EQ(reader.stats().lines_read, reader.stats().csv_records_accepted + reader.stats().csv_records_rejected);
    EXPECT_EQ(reader.stats().csv_records_accepted, reader.stats().ticks_emitted + reader.stats().tick_conversion_rejected);
}

TEST(DatasetReaderTest, MissingDataset)
{
    auto path = fixture_dir / "missing_dataset.csv";
    DatasetReader reader(path.string());
    EXPECT_FALSE(reader.is_valid());
}