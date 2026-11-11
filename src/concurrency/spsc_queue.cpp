#include "hft/concurrency/spsc_queue.hpp"

#include <cstddef>
#include <limits>
#include <new>
#include <stdexcept>

namespace hft::concurrency::detail {

namespace {

std::size_t effective_alignment(std::size_t alignment)
{
    return alignment < alignof(void*) ? alignof(void*) : alignment;
}

std::size_t align_up(std::size_t value, std::size_t alignment)
{
    const std::size_t remainder = value % alignment;
    if (remainder == 0) {
        return value;
    }

    const std::size_t increment = alignment - remainder;
    if (value > std::numeric_limits<std::size_t>::max() - increment) {
        throw std::invalid_argument("SPSCQueue storage size overflows");
    }
    return value + increment;
}

std::size_t calculate_stride(
    std::size_t element_size,
    std::size_t element_alignment
)
{
    if (element_size == 0) {
        throw std::invalid_argument("SPSCQueue element size must be positive");
    }
    if (element_alignment == 0 ||
        (element_alignment & (element_alignment - 1)) != 0) {
        throw std::invalid_argument(
            "SPSCQueue element alignment must be a power of two"
        );
    }

    return align_up(element_size, effective_alignment(element_alignment));
}

} // namespace

std::byte* SPSCQueueCore::slot_address(
    const State& state,
    std::size_t position
) noexcept
{
    return state.storage + (position % state.capacity) * state.stride;
}

SPSCQueueCore::SPSCQueueCore(
    std::size_t element_size,
    std::size_t element_alignment,
    std::size_t capacity
)
    : mState(new State{
          element_size,
          element_alignment,
          calculate_stride(element_size, element_alignment),
          capacity,
          nullptr,
          0,
          0,
          0,
          0,
          0,
          0
      })
{
    if (mState->stride >
        std::numeric_limits<std::size_t>::max() / capacity) {
        delete mState;
        mState = nullptr;
        throw std::invalid_argument("SPSCQueue storage size overflows");
    }

    try {
        mState->storage = static_cast<std::byte*>(
            ::operator new(
                mState->stride * capacity,
                std::align_val_t(effective_alignment(element_alignment))
            )
        );
    } catch (...) {
        delete mState;
        mState = nullptr;
        throw;
    }
}

SPSCQueueCore::~SPSCQueueCore()
{
    if (mState == nullptr) {
        return;
    }

    ::operator delete(
        mState->storage,
        std::align_val_t(effective_alignment(mState->element_alignment))
    );
    delete mState;
}

void* SPSCQueueCore::try_reserve_push() noexcept
{
    const std::size_t write = mState->write_position;
    const std::size_t read = mState->published_read.load(
        std::memory_order_acquire
    );

    if (write - read >= mState->capacity) {
        return nullptr;
    }

    mState->reserved_write = write;
    return slot_address(*mState, write);
}

void SPSCQueueCore::publish_push() noexcept
{
    const std::size_t write = mState->reserved_write;
    mState->write_position = write + 1;
    // Construction happens-before the consumer observes this release.
    mState->published_write.store(
        mState->write_position,
        std::memory_order_release
    );
}

void SPSCQueueCore::cancel_push() noexcept
{
}

void* SPSCQueueCore::try_reserve_pop() noexcept
{
    const std::size_t read = mState->read_position;
    const std::size_t write = mState->published_write.load(
        std::memory_order_acquire
    );

    if (read == write) {
        return nullptr;
    }

    mState->reserved_read = read;
    return slot_address(*mState, read);
}

void SPSCQueueCore::publish_pop() noexcept
{
    const std::size_t read = mState->reserved_read;
    mState->read_position = read + 1;
    // Destruction happens-before the producer observes this release.
    mState->published_read.store(
        mState->read_position,
        std::memory_order_release
    );
}

bool SPSCQueueCore::empty() const noexcept
{
    return mState->published_read.load(std::memory_order_acquire) ==
           mState->published_write.load(
        std::memory_order_acquire
    );
}

bool SPSCQueueCore::full() const noexcept
{
    return mState->published_write.load(std::memory_order_acquire) -
               mState->published_read.load(std::memory_order_acquire) >=
           mState->capacity;
}

std::size_t SPSCQueueCore::size() const noexcept
{
    return mState->published_write.load(std::memory_order_acquire) -
           mState->published_read.load(std::memory_order_acquire);
}

} // namespace hft::concurrency::detail
