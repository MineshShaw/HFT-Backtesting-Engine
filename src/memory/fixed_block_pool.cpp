#include "hft/memory/fixed_block_pool.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <optional>
#include <stdexcept>
#include <vector>

namespace hft::memory {

struct FixedBlockPool::State {
    std::size_t block_size;
    std::size_t block_alignment;
    std::size_t stride;
    std::size_t capacity;
    std::size_t available;
    std::byte* storage{};
    std::size_t free_head{
        std::numeric_limits<std::size_t>::max()
    };
    std::vector<bool> free_slots;
};

constexpr std::size_t no_slot = std::numeric_limits<std::size_t>::max();

std::size_t FixedBlockPool::effective_alignment(std::size_t alignment)
{
    return alignment < alignof(void*) ? alignof(void*) : alignment;
}

void FixedBlockPool::validate_configuration(
    std::size_t block_size,
    std::size_t block_alignment
)
{
    if (block_size == 0) {
        throw std::invalid_argument("FixedBlockPool block size must be positive");
    }
    if (block_alignment == 0 || !std::has_single_bit(block_alignment)) {
        throw std::invalid_argument(
            "FixedBlockPool alignment must be a power of two"
        );
    }
}

std::size_t FixedBlockPool::align_up(
    std::size_t value,
    std::size_t alignment
)
{
    const std::size_t remainder = value % alignment;
    if (remainder == 0) {
        return value;
    }

    const std::size_t increment = alignment - remainder;
    if (value > std::numeric_limits<std::size_t>::max() - increment) {
        throw std::invalid_argument("FixedBlockPool stride overflows");
    }
    return value + increment;
}

std::size_t FixedBlockPool::calculate_stride(
    std::size_t block_size,
    std::size_t block_alignment
)
{
    validate_configuration(block_size, block_alignment);
    const std::size_t minimum_size =
        block_size < sizeof(std::size_t) ? sizeof(std::size_t) : block_size;
    return align_up(minimum_size, effective_alignment(block_alignment));
}

std::size_t& FixedBlockPool::next_slot(
    State& state,
    std::size_t index
) noexcept
{
    return *reinterpret_cast<std::size_t*>(
        state.storage + index * state.stride
    );
}

void* FixedBlockPool::block_address(
    const State& state,
    std::size_t index
) noexcept
{
    return state.storage + index * state.stride;
}

std::optional<std::size_t> FixedBlockPool::block_index(
    const State& state,
    const void* pointer
) noexcept
{
    if (pointer == nullptr || state.storage == nullptr) {
        return std::nullopt;
    }

    const auto storage_address =
        reinterpret_cast<std::uintptr_t>(state.storage);
    const auto pointer_address =
        reinterpret_cast<std::uintptr_t>(pointer);

    if (pointer_address < storage_address) {
        return std::nullopt;
    }

    const std::uintptr_t offset = pointer_address - storage_address;
    if (offset % state.stride != 0 ||
        offset / state.stride >= state.capacity) {
        return std::nullopt;
    }

    return static_cast<std::size_t>(offset / state.stride);
}

FixedBlockPool::FixedBlockPool(
    std::size_t block_size,
    std::size_t block_alignment,
    std::size_t capacity
)
    : mState(new State{
          block_size,
          block_alignment,
          calculate_stride(block_size, block_alignment),
          capacity,
          capacity,
          nullptr,
          no_slot,
          std::vector<bool>(capacity, false)
      })
{
    if (capacity == 0) {
        return;
    }

    if (mState->stride >
        std::numeric_limits<std::size_t>::max() / capacity) {
        delete mState;
        mState = nullptr;
        throw std::invalid_argument("FixedBlockPool storage size overflows");
    }

    try {
        mState->storage = static_cast<std::byte*>(
            ::operator new(
                mState->stride * capacity,
                std::align_val_t(effective_alignment(block_alignment))
            )
        );
    } catch (...) {
        delete mState;
        mState = nullptr;
        throw;
    }

    for (std::size_t index = 0; index + 1 < capacity; ++index) {
        next_slot(*mState, index) = index + 1;
    }
    next_slot(*mState, capacity - 1) = no_slot;
    mState->free_head = 0;
}

FixedBlockPool::~FixedBlockPool()
{
    if (mState == nullptr) {
        return;
    }

    if (mState->storage != nullptr) {
        ::operator delete(
            mState->storage,
            std::align_val_t(effective_alignment(mState->block_alignment))
        );
    }
    delete mState;
}

void* FixedBlockPool::allocate() noexcept
{
    if (mState->free_head == no_slot) {
        return nullptr;
    }

    const std::size_t index = mState->free_head;
    mState->free_head = next_slot(*mState, index);
    mState->free_slots[index] = true;
    --mState->available;
    return block_address(*mState, index);
}

bool FixedBlockPool::deallocate(void* pointer) noexcept
{
    const auto index = block_index(*mState, pointer);
    if (!index.has_value() || !mState->free_slots[*index]) {
        return false;
    }

    mState->free_slots[*index] = false;
    next_slot(*mState, *index) = mState->free_head;
    mState->free_head = *index;
    ++mState->available;
    return true;
}

bool FixedBlockPool::owns_allocated_block(const void* pointer) const noexcept
{
    const auto index = block_index(*mState, pointer);
    return index.has_value() && mState->free_slots[*index];
}

std::size_t FixedBlockPool::block_size() const noexcept
{
    return mState->block_size;
}

std::size_t FixedBlockPool::block_alignment() const noexcept
{
    return mState->block_alignment;
}

std::size_t FixedBlockPool::capacity() const noexcept
{
    return mState->capacity;
}

std::size_t FixedBlockPool::available_blocks() const noexcept
{
    return mState->available;
}

} // namespace hft::memory
