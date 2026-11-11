#include "hft/concurrency/spsc_queue.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <thread>

namespace {

struct Tracked {
    static inline std::atomic<int> live_count{0};
    static inline std::atomic<int> destroyed_count{0};

    explicit Tracked(int value) : value(value)
    {
        ++live_count;
    }

    Tracked(const Tracked& other) : value(other.value)
    {
        ++live_count;
    }

    Tracked(Tracked&& other) noexcept : value(other.value)
    {
        ++live_count;
        other.value = -1;
    }

    Tracked& operator=(const Tracked& other) noexcept
    {
        value = other.value;
        return *this;
    }

    Tracked& operator=(Tracked&& other) noexcept
    {
        value = other.value;
        other.value = -1;
        return *this;
    }

    ~Tracked()
    {
        --live_count;
        ++destroyed_count;
    }

    int value;
};

struct MoveOnly {
    explicit MoveOnly(int value) : value(value) {}
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&& other) noexcept : value(other.value)
    {
        other.value = -1;
    }
    MoveOnly& operator=(MoveOnly&& other) noexcept
    {
        value = other.value;
        other.value = -1;
        return *this;
    }

    int value;
};

struct ThrowingConstructor {
    explicit ThrowingConstructor(bool should_throw)
    {
        if (should_throw) {
            throw std::runtime_error("construction failed");
        }
    }

    ThrowingConstructor& operator=(ThrowingConstructor&&) noexcept = default;
};

} // namespace

TEST(SPSCQueueTest, StartsEmptyAndUsesFullCapacity)
{
    hft::concurrency::SPSCQueue<int, 3> queue;

    EXPECT_TRUE(queue.empty());
    EXPECT_FALSE(queue.full());
    EXPECT_EQ(queue.size(), 0);
    EXPECT_EQ(queue.capacity(), 3);

    EXPECT_TRUE(queue.try_push(1));
    EXPECT_TRUE(queue.try_push(2));
    EXPECT_TRUE(queue.try_push(3));
    EXPECT_FALSE(queue.try_push(4));
    EXPECT_TRUE(queue.full());
    EXPECT_EQ(queue.size(), 3);
}

TEST(SPSCQueueTest, PreservesFifoOrderAndWraps)
{
    hft::concurrency::SPSCQueue<int, 2> queue;
    int output = 0;

    for (int value = 0; value < 100; ++value) {
        ASSERT_TRUE(queue.try_push(value));
        ASSERT_TRUE(queue.try_pop(output));
        EXPECT_EQ(output, value);
    }

    EXPECT_TRUE(queue.empty());
}

TEST(SPSCQueueTest, SupportsMoveOnlyValues)
{
    hft::concurrency::SPSCQueue<MoveOnly, 2> queue;
    auto value = std::make_unique<int>(42);
    MoveOnly input(*value);

    ASSERT_TRUE(queue.try_push(std::move(input)));
    MoveOnly output(0);
    ASSERT_TRUE(queue.try_pop(output));
    EXPECT_EQ(output.value, 42);
}

TEST(SPSCQueueTest, ConstructsAndDestroysNonTrivialValues)
{
    Tracked::live_count = 0;
    Tracked::destroyed_count = 0;

    {
        hft::concurrency::SPSCQueue<Tracked, 2> queue;
        ASSERT_TRUE(queue.try_emplace(1));
        ASSERT_TRUE(queue.try_emplace(2));
        EXPECT_EQ(Tracked::live_count, 2);

        Tracked output(0);
        ASSERT_TRUE(queue.try_pop(output));
        EXPECT_EQ(output.value, 1);
        EXPECT_EQ(Tracked::live_count, 2);
    }

    EXPECT_EQ(Tracked::live_count, 0);
    EXPECT_EQ(Tracked::destroyed_count, 3);
}

TEST(SPSCQueueTest, ConstructionExceptionLeavesQueueUnchanged)
{
    hft::concurrency::SPSCQueue<ThrowingConstructor, 1> queue;

    EXPECT_THROW(queue.try_emplace(true), std::runtime_error);
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
    EXPECT_TRUE(queue.try_emplace(false));
    EXPECT_TRUE(queue.full());
}

TEST(SPSCQueueTest, RepeatedFullAndEmptyTransitions)
{
    hft::concurrency::SPSCQueue<int, 4> queue;
    int output = 0;

    for (int round = 0; round < 100; ++round) {
        for (int value = 0; value < 4; ++value) {
            ASSERT_TRUE(queue.try_push(round * 4 + value));
        }
        ASSERT_TRUE(queue.full());
        ASSERT_FALSE(queue.try_push(0));

        for (int value = 0; value < 4; ++value) {
            ASSERT_TRUE(queue.try_pop(output));
            EXPECT_EQ(output, round * 4 + value);
        }
        ASSERT_TRUE(queue.empty());
        ASSERT_FALSE(queue.try_pop(output));
    }
}

TEST(SPSCQueueTest, ConcurrentProducerAndConsumerPreserveSequence)
{
    constexpr int count = 200000;
    hft::concurrency::SPSCQueue<int, 1024> queue;
    std::atomic<bool> producer_done{false};
    std::atomic<int> consumed{0};
    std::atomic<bool> sequence_error{false};

    std::thread producer([&] {
        for (int value = 0; value < count;) {
            if (queue.try_push(value)) {
                ++value;
            } else {
                std::this_thread::yield();
            }
        }
        producer_done.store(true, std::memory_order_release);
    });

    std::thread consumer([&] {
        int expected = 0;
        int output = 0;
        while (!producer_done.load(std::memory_order_acquire) ||
               !queue.empty()) {
            if (queue.try_pop(output)) {
                if (output != expected) {
                    sequence_error.store(true, std::memory_order_relaxed);
                }
                ++expected;
                consumed.store(expected, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
        if (expected != count) {
            sequence_error.store(true, std::memory_order_relaxed);
        }
    });

    producer.join();
    consumer.join();
    EXPECT_FALSE(sequence_error.load(std::memory_order_relaxed));
    EXPECT_EQ(consumed.load(), count);
}

