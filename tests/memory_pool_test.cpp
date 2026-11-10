#include "hft/memory/fixed_block_pool.hpp"
#include "hft/memory/object_pool.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <new>
#include <stdexcept>

namespace {

struct Tracked {
    static inline int live_count = 0;
    static inline int destroyed_count = 0;

    explicit Tracked(int value) : value(value)
    {
        ++live_count;
    }

    ~Tracked()
    {
        --live_count;
        ++destroyed_count;
    }

    int value;
};

struct Throwing {
    explicit Throwing(bool should_throw)
    {
        if (should_throw) {
            throw std::runtime_error("construction failed");
        }
    }
};

struct alignas(64) OverAligned {
    std::uint64_t value{};
};

} // namespace

TEST(FixedBlockPoolTest, AllocatesToCapacityAndReusesReleasedBlocks)
{
    hft::memory::FixedBlockPool pool(sizeof(std::uint64_t), alignof(std::uint64_t), 2);

    EXPECT_EQ(pool.capacity(), 2);
    EXPECT_EQ(pool.available_blocks(), 2);

    void* first = pool.allocate();
    void* second = pool.allocate();
    EXPECT_NE(first, nullptr);
    EXPECT_NE(second, nullptr);
    EXPECT_NE(first, second);
    EXPECT_EQ(pool.available_blocks(), 0);
    EXPECT_EQ(pool.allocate(), nullptr);

    EXPECT_TRUE(pool.deallocate(first));
    EXPECT_EQ(pool.available_blocks(), 1);
    void* reused = pool.allocate();
    EXPECT_EQ(reused, first);
    EXPECT_EQ(pool.available_blocks(), 0);
}

TEST(FixedBlockPoolTest, RejectsInvalidAndRepeatedDeallocation)
{
    hft::memory::FixedBlockPool pool(sizeof(std::uint64_t), alignof(std::uint64_t), 1);
    void* block = pool.allocate();
    std::uint64_t foreign{};

    EXPECT_FALSE(pool.deallocate(nullptr));
    EXPECT_FALSE(pool.deallocate(&foreign));
    EXPECT_FALSE(pool.deallocate(static_cast<std::byte*>(block) + 1));
    EXPECT_TRUE(pool.deallocate(block));
    EXPECT_FALSE(pool.deallocate(block));
    EXPECT_EQ(pool.available_blocks(), 1);
}

TEST(FixedBlockPoolTest, SupportsOverAlignedBlocks)
{
    hft::memory::FixedBlockPool pool(sizeof(OverAligned), alignof(OverAligned), 2);

    void* first = pool.allocate();
    void* second = pool.allocate();

    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(first) % alignof(OverAligned), 0U);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(second) % alignof(OverAligned), 0U);
    EXPECT_TRUE(pool.deallocate(first));
    EXPECT_TRUE(pool.deallocate(second));
}

TEST(FixedBlockPoolTest, HandlesZeroCapacityAndInvalidConfiguration)
{
    hft::memory::FixedBlockPool empty(sizeof(int), alignof(int), 0);
    EXPECT_EQ(empty.capacity(), 0);
    EXPECT_EQ(empty.available_blocks(), 0);
    EXPECT_EQ(empty.allocate(), nullptr);

    EXPECT_THROW(
        hft::memory::FixedBlockPool(0, alignof(int), 1),
        std::invalid_argument
    );
    EXPECT_THROW(
        hft::memory::FixedBlockPool(sizeof(int), 3, 1),
        std::invalid_argument
    );
}

TEST(ObjectPoolTest, ConstructsDestroysAndRejectsInvalidDestruction)
{
    Tracked::live_count = 0;
    Tracked::destroyed_count = 0;
    hft::memory::ObjectPool<Tracked> pool(2);

    Tracked* first = pool.create(10);
    Tracked* second = pool.create(20);
    EXPECT_EQ(pool.available_objects(), 0);
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(Tracked::live_count, 2);
    EXPECT_EQ(first->value, 10);

    EXPECT_FALSE(pool.destroy(nullptr));
    EXPECT_TRUE(pool.destroy(first));
    EXPECT_EQ(Tracked::live_count, 1);
    EXPECT_EQ(Tracked::destroyed_count, 1);
    EXPECT_FALSE(pool.destroy(first));
    EXPECT_EQ(pool.available_objects(), 1);
    EXPECT_TRUE(pool.destroy(second));
    EXPECT_EQ(Tracked::live_count, 0);
    EXPECT_EQ(Tracked::destroyed_count, 2);
}

TEST(ObjectPoolTest, ReleasesSlotWhenConstructionThrows)
{
    hft::memory::ObjectPool<Throwing> pool(1);

    EXPECT_THROW(pool.create(true), std::runtime_error);
    EXPECT_EQ(pool.available_objects(), 1);

    Throwing* object = pool.create(false);
    ASSERT_NE(object, nullptr);
    EXPECT_EQ(pool.available_objects(), 0);
    EXPECT_TRUE(pool.destroy(object));
}

TEST(ObjectPoolTest, SupportsOverAlignedObjects)
{
    hft::memory::ObjectPool<OverAligned> pool(1);

    OverAligned* object = pool.create();
    ASSERT_NE(object, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(object) % alignof(OverAligned), 0U);
    EXPECT_TRUE(pool.destroy(object));
}

