#pragma once

#include <atomic>
#include <cstddef>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace hft::concurrency::detail {

class SPSCQueueCore {
public:
    SPSCQueueCore(
        std::size_t element_size,
        std::size_t element_alignment,
        std::size_t capacity
    );

    ~SPSCQueueCore();

    SPSCQueueCore(const SPSCQueueCore&) = delete;
    SPSCQueueCore& operator=(const SPSCQueueCore&) = delete;
    SPSCQueueCore(SPSCQueueCore&&) = delete;
    SPSCQueueCore& operator=(SPSCQueueCore&&) = delete;

    void* try_reserve_push() noexcept;
    void publish_push() noexcept;
    void cancel_push() noexcept;

    void* try_reserve_pop() noexcept;
    void publish_pop() noexcept;

    bool empty() const noexcept;
    bool full() const noexcept;
    std::size_t size() const noexcept;

private:
    struct State {
        std::size_t element_size;
        std::size_t element_alignment;
        std::size_t stride;
        std::size_t capacity;
        std::byte* storage{};
        std::size_t reserved_write{};
        std::size_t reserved_read{};
        std::size_t write_position{};
        std::size_t read_position{};
        alignas(64) std::atomic<std::size_t> published_write{0};
        alignas(64) std::atomic<std::size_t> published_read{0};
    };

    static std::byte* slot_address(
        const State& state,
        std::size_t position
    ) noexcept;

    State* mState;
};

} // namespace hft::concurrency::detail

namespace hft::concurrency {

template <typename T, std::size_t Capacity>
class SPSCQueue {
    static_assert(Capacity > 0, "SPSCQueue capacity must be positive");
    static_assert(
        Capacity <= std::numeric_limits<std::size_t>::max() / 2,
        "SPSCQueue capacity is too large for safe position arithmetic"
    );

public:
    SPSCQueue()
        : mCore(sizeof(T), alignof(T), Capacity)
    {
    }

    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;
    SPSCQueue(SPSCQueue&&) = delete;
    SPSCQueue& operator=(SPSCQueue&&) = delete;

    ~SPSCQueue()
    {
        while (T* value = static_cast<T*>(mCore.try_reserve_pop())) {
            std::destroy_at(value);
            mCore.publish_pop();
        }
    }

    template <typename... Args>
    bool try_emplace(Args&&... args)
    {
        void* storage = mCore.try_reserve_push();
        if (storage == nullptr) {
            return false;
        }

        try {
            std::construct_at(
                static_cast<T*>(storage),
                std::forward<Args>(args)...
            );
        } catch (...) {
            mCore.cancel_push();
            throw;
        }

        mCore.publish_push();
        return true;
    }

    bool try_push(const T& value)
    {
        return try_emplace(value);
    }

    bool try_push(T&& value)
    {
        return try_emplace(std::move(value));
    }

    bool try_pop(T& output)
    {
        static_assert(
            std::is_nothrow_move_assignable_v<T> ||
            std::is_nothrow_copy_assignable_v<T>,
            "SPSCQueue::try_pop requires a non-throwing assignment operation"
        );

        T* value = static_cast<T*>(mCore.try_reserve_pop());
        if (value == nullptr) {
            return false;
        }

        if constexpr (std::is_nothrow_move_assignable_v<T>) {
            output = std::move(*value);
        } else {
            output = *value;
        }

        std::destroy_at(value);
        mCore.publish_pop();
        return true;
    }

    bool empty() const noexcept { return mCore.empty(); }
    bool full() const noexcept { return mCore.full(); }
    std::size_t size() const noexcept { return mCore.size(); }

    static constexpr std::size_t capacity() noexcept
    {
        return Capacity;
    }

private:
    detail::SPSCQueueCore mCore;
};

} // namespace hft::concurrency
