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

    EXPECT_EQ(count, 6); 
    EXPECT_EQ(reader.stats().lines_read, 6);
    EXPECT_EQ(reader.stats().csv_records_accepted, 6);
    EXPECT_EQ(reader.stats().csv_records_rejected, 0);
    EXPECT_EQ(reader.stats().ticks_emitted, 6);
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
    EXPECT_EQ(reader.stats().lines_read, 11); 
    EXPECT_EQ(reader.stats().csv_records_accepted, 6);
    EXPECT_EQ(reader.stats().csv_records_rejected, 5);
    EXPECT_EQ(reader.stats().ticks_emitted, 5);
    EXPECT_EQ(reader.stats().tick_conversion_rejected, 1);

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

TEST(DatasetReaderTest, MoveConstructionTransfersReaderState)
{
    auto test_file_path = fixture_dir / "mixed_dataset.csv";
    DatasetReader source{test_file_path};
    ASSERT_TRUE(source.is_valid());

    Tick tick;
    EXPECT_TRUE(source.next(tick)); // Advance to the next record to simulate reading.

    const int64_t initial_lines_read = source.stats().lines_read;

    DatasetReader destination{std::move(source)};

    EXPECT_TRUE(destination.is_valid());
    EXPECT_EQ(destination.stats().lines_read, initial_lines_read); // State should be transferred.
    
    std::size_t count = 0;
    while (destination.next(tick)) {
        count++;
    }

    EXPECT_FALSE(destination.next(tick)); // Should reach the end of the dataset.

    EXPECT_EQ(count, 4); // Already read one record before the loop, so 4 more should be read.
    EXPECT_EQ(destination.stats().lines_read, 11);
    EXPECT_EQ(destination.stats().csv_records_accepted, 6);
    EXPECT_EQ(destination.stats().csv_records_rejected, 5);
    EXPECT_EQ(destination.stats().ticks_emitted, 5);
    EXPECT_EQ(destination.stats().tick_conversion_rejected, 1);

    EXPECT_EQ(destination.stats().lines_read, destination.stats().csv_records_accepted + destination.stats().csv_records_rejected);
    EXPECT_EQ(destination.stats().csv_records_accepted, destination.stats().ticks_emitted + destination.stats().tick_conversion_rejected);
    
    EXPECT_FALSE(source.is_valid()); // Source should no longer be valid after move.

    // Source must be safe to destroy.
}

TEST(DatasetReaderTest, MoveAssignmentReleasesOldDestinationMapping)
{
    auto source_file_path = fixture_dir / "valid_dataset.csv";
    auto destination_file_path = fixture_dir / "mixed_dataset.csv";

    DatasetReader source{source_file_path};
    DatasetReader destination{destination_file_path};

    ASSERT_TRUE(source.is_valid());
    ASSERT_TRUE(destination.is_valid());

    Tick sourceTick;
    Tick destinationTick;

    ASSERT_TRUE(source.next(sourceTick)); // Advance source to simulate reading.
    ASSERT_TRUE(destination.next(destinationTick)); // Advance destination to simulate reading.
    
    destination = std::move(source);

    EXPECT_TRUE(destination.is_valid());    
    EXPECT_FALSE(source.is_valid());

    EXPECT_EQ(destination.stats().lines_read, 1); // Source had read one line before the move.

    EXPECT_EQ(source.stats().lines_read, 0); // Moved-from source should have zeroed statistics.

    while (destination.next(destinationTick)) {}

    EXPECT_EQ(destinationTick.timestamp, 1); // Last valid record from the source dataset.
    EXPECT_EQ(destinationTick.symbol, "TEST");
    EXPECT_EQ(destinationTick.price, 1.0);
    EXPECT_EQ(destinationTick.size, 1);
}

TEST(DatasetReaderTest, SelfMoveAssignmentIsSafe)
{
    auto test_file_path = fixture_dir / "valid_dataset.csv";
    DatasetReader reader{test_file_path};
    Tick tick;

    ASSERT_TRUE(reader.is_valid());
    ASSERT_TRUE(reader.next(tick)); // Advance to the next record to simulate reading.

    reader = std::move(reader);

    EXPECT_TRUE(reader.is_valid());
    EXPECT_TRUE(reader.next(tick)); // Should still be able to read after self-move assignment.
}

TEST(DatasetReaderTest, MoveAssignmentIntoEmptyDestination)
{
    auto source_file_path = fixture_dir / "valid_dataset.csv";
    auto destination_file_path = fixture_dir / "invalid_dataset.csv";

    DatasetReader source{source_file_path};
    DatasetReader destination{destination_file_path}; // Invalid dataset, destination is not valid.

    ASSERT_TRUE(source.is_valid());
    ASSERT_FALSE(destination.is_valid());

    destination = std::move(source);

    EXPECT_TRUE(destination.is_valid());
    EXPECT_FALSE(source.is_valid()); // Source should no longer be valid after move.

    Tick tick;
    ASSERT_TRUE(destination.next(tick)); // Should be able to read from the moved destination.

    EXPECT_EQ(source.stats().lines_read, 0);
    EXPECT_EQ(source.stats().ticks_emitted, 0);

    EXPECT_EQ(tick.timestamp, 1609459200);
    EXPECT_EQ(tick.symbol, "AAPL");
    EXPECT_EQ(tick.price, 130.0);
    EXPECT_EQ(tick.size, 100);
}

TEST(DatasetReaderTest, DestinationRemainsValidAfterSourceDestruction)
{
    std::unique_ptr<DatasetReader> destination;

    {
        DatasetReader source{fixture_dir / "valid_dataset.csv"};
        ASSERT_TRUE(source.is_valid());

        Tick tick;
        ASSERT_TRUE(source.next(tick));

        destination = std::make_unique<DatasetReader>(std::move(source));
    }

    ASSERT_TRUE(destination->is_valid());

    Tick tick;
    EXPECT_TRUE(destination->next(tick));
    EXPECT_EQ(tick.symbol, "AAPL");
}