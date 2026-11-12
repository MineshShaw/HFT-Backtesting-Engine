#include "hft/replay/event_loop.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

class CsvFixture : public ::testing::Test {
protected:
    std::filesystem::path path =
        std::filesystem::temp_directory_path() / "hft_replay_test.csv";

    void TearDown() override
    {
        std::error_code error;
        std::filesystem::remove(path, error);
    }

    void write_csv(const std::string& content)
    {
        std::ofstream file(path);
        ASSERT_TRUE(file.is_open());
        file << content;
        ASSERT_TRUE(file.good());
    }
};

void write_u16(
    std::vector<std::byte>& data,
    std::size_t offset,
    std::uint16_t value
)
{
    data[offset] = static_cast<std::byte>(value & 0xFFU);
    data[offset + 1] = static_cast<std::byte>(value >> 8);
}

void write_u32(
    std::vector<std::byte>& data,
    std::size_t offset,
    std::uint32_t value
)
{
    for (std::size_t index = 0; index < 4; ++index) {
        data[offset + index] =
            static_cast<std::byte>((value >> (index * 8)) & 0xFFU);
    }
}

void write_u64(
    std::vector<std::byte>& data,
    std::size_t offset,
    std::uint64_t value
)
{
    for (std::size_t index = 0; index < 8; ++index) {
        data[offset + index] =
            static_cast<std::byte>((value >> (index * 8)) & 0xFFU);
    }
}

TEST_F(CsvFixture, ReplaysBinaryThroughTheSamePipeline)
{
    const auto binary_path = path.string() + ".bin";
    std::vector<std::byte> file(48 + 6 + 24);
    const std::array<std::uint8_t, 8> magic{
        'H', 'F', 'T', 'B', 'D', 'A', 'T', '\0'
    };
    for (std::size_t index = 0; index < magic.size(); ++index) {
        file[index] = static_cast<std::byte>(magic[index]);
    }
    write_u16(file, 8, 1);
    write_u16(file, 10, 0);
    write_u32(file, 12, 54);
    write_u32(file, 16, 24);
    write_u64(file, 24, 1);
    write_u32(file, 32, 1);
    write_u32(file, 36, 6);
    write_u32(file, 40, 100);
    write_u32(file, 44, 10);
    write_u16(file, 48, 4);
    file[50] = std::byte{'A'};
    file[51] = std::byte{'A'};
    file[52] = std::byte{'P'};
    file[53] = std::byte{'L'};
    write_u64(file, 54, 7);
    write_u64(file, 62, 12345);
    write_u16(file, 70, 0);
    write_u32(file, 74, 25);

    {
        std::ofstream output(binary_path, std::ios::binary);
        ASSERT_TRUE(output.is_open());
        output.write(
            reinterpret_cast<const char*>(file.data()),
            static_cast<std::streamsize>(file.size())
        );
        ASSERT_TRUE(output.good());
    }

    auto source = std::make_shared<hft::replay::BinaryDataSource>(binary_path);
    std::vector<hft::data::Tick> ticks;
    hft::replay::EventLoop loop(
        hft::replay::ReplaySource::from_owned(std::move(source)),
        [&](const hft::data::Tick& tick) {
            ticks.push_back(tick);
        }
    );

    ASSERT_TRUE(loop.start());
    loop.join();

    ASSERT_EQ(ticks.size(), 1);
    EXPECT_EQ(ticks[0].timestamp, 7);
    EXPECT_EQ(ticks[0].symbol, "AAPL");
    EXPECT_DOUBLE_EQ(ticks[0].price, 123.45);
    EXPECT_DOUBLE_EQ(ticks[0].size, 2.5);
    std::error_code error;
    std::filesystem::remove(binary_path, error);
}

TEST_F(CsvFixture, ReplaysCsvInOrder)
{
    write_csv(
        "1,AAPL,10.0,1\n"
        "2,MSFT,20.0,2\n"
        "3,GOOG,30.0,3\n"
    );

    auto source = std::make_shared<hft::replay::CsvDataSource>(path.string());
    hft::replay::EventLoop loop(
        hft::replay::ReplaySource::from_owned(std::move(source)),
        [](const hft::data::Tick& tick) {
            static_cast<void>(tick);
        }
    );

    ASSERT_TRUE(loop.start());
    loop.join();

    EXPECT_EQ(loop.stats().events_produced, 3);
    EXPECT_EQ(loop.stats().events_consumed, 3);
    EXPECT_EQ(loop.stats().shutdown_reason, hft::replay::ShutdownReason::Completed);
    EXPECT_FALSE(loop.error().has_value());
}

TEST_F(CsvFixture, DrainsQueueBeforeSourceLifetimeEnds)
{
    write_csv(
        "1,AAPL,10.0,1\n"
        "2,MSFT,20.0,2\n"
    );

    auto source = std::make_shared<hft::replay::CsvDataSource>(path.string());
    std::vector<std::string> symbols;
    std::mutex mutex;
    hft::replay::EventLoop loop(
        hft::replay::ReplaySource::from_owned(std::move(source)),
        [&](const hft::data::Tick& tick) {
            std::scoped_lock lock(mutex);
            symbols.emplace_back(tick.symbol);
        }
    );

    ASSERT_TRUE(loop.start());
    loop.join();

    ASSERT_EQ(symbols.size(), 2);
    EXPECT_EQ(symbols[0], "AAPL");
    EXPECT_EQ(symbols[1], "MSFT");
}

TEST_F(CsvFixture, PropagatesSourceInitializationFailure)
{
    std::error_code error;
    std::filesystem::remove(path, error);

    auto source = std::make_shared<hft::replay::CsvDataSource>(path.string());
    hft::replay::EventLoop loop(
        hft::replay::ReplaySource::from_owned(std::move(source)),
        [](const hft::data::Tick&) {}
    );

    EXPECT_FALSE(loop.start());
    EXPECT_EQ(loop.stats().source_errors, 0);
    EXPECT_EQ(
        loop.stats().shutdown_reason,
        hft::replay::ShutdownReason::SourceError
    );
    EXPECT_TRUE(loop.error().has_value());
}

struct TestSource {
    bool valid{true};
    int next_calls{};

    bool is_valid() const { return valid; }

    hft::replay::ReadStatus next(hft::data::Tick&)
    {
        ++next_calls;
        return hft::replay::ReadStatus::Error;
    }
};

TEST(EventLoopTest, PropagatesRuntimeSourceError)
{
    auto source = std::make_shared<TestSource>();
    hft::replay::EventLoop loop(
        hft::replay::ReplaySource::from_owned(std::move(source)),
        [](const hft::data::Tick&) {}
    );

    ASSERT_TRUE(loop.start());
    loop.join();

    EXPECT_EQ(loop.stats().source_errors, 1);
    EXPECT_EQ(loop.stats().events_produced, 0);
    EXPECT_EQ(
        loop.stats().shutdown_reason,
        hft::replay::ShutdownReason::SourceError
    );
    ASSERT_TRUE(loop.error().has_value());
}

TEST_F(CsvFixture, PropagatesConsumerException)
{
    write_csv("1,AAPL,10.0,1\n");

    auto source = std::make_shared<hft::replay::CsvDataSource>(path.string());
    hft::replay::EventLoop loop(
        hft::replay::ReplaySource::from_owned(std::move(source)),
        [](const hft::data::Tick&) {
            throw std::runtime_error("consumer failed");
        }
    );

    ASSERT_TRUE(loop.start());
    loop.join();

    EXPECT_EQ(loop.stats().consumer_errors, 1);
    EXPECT_EQ(
        loop.stats().shutdown_reason,
        hft::replay::ShutdownReason::ConsumerError
    );
    ASSERT_TRUE(loop.error().has_value());
    EXPECT_EQ(loop.error().value(), "consumer failed");
}

TEST_F(CsvFixture, SupportsRequestedShutdown)
{
    std::string content;
    for (int index = 0; index < 2000; ++index) {
        content += std::to_string(index) + ",AAPL,10.0,1\n";
    }
    write_csv(content);

    auto source = std::make_shared<hft::replay::CsvDataSource>(path.string());
    hft::replay::EventLoop loop(
        hft::replay::ReplaySource::from_owned(std::move(source)),
        [](const hft::data::Tick&) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    );

    ASSERT_TRUE(loop.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    loop.request_shutdown();
    loop.join();

    EXPECT_EQ(
        loop.stats().shutdown_reason,
        hft::replay::ShutdownReason::Requested
    );
    EXPECT_LE(loop.stats().events_consumed, loop.stats().events_produced);
}

} // namespace
