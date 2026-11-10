#pragma once

#include <cstddef>
#include <optional>

namespace hft::memory {

// Single-threaded raw storage pool. The pool owns its backing storage, while
// callers own the lifetime of objects constructed in returned blocks.
class FixedBlockPool {
public:
    FixedBlockPool(
        std::size_t block_size,
        std::size_t block_alignment,
        std::size_t capacity
    );

    ~FixedBlockPool();

    FixedBlockPool(const FixedBlockPool&) = delete;
    FixedBlockPool& operator=(const FixedBlockPool&) = delete;
    FixedBlockPool(FixedBlockPool&&) = delete;
    FixedBlockPool& operator=(FixedBlockPool&&) = delete;

    void* allocate() noexcept;
    bool deallocate(void* pointer) noexcept;
    bool owns_allocated_block(const void* pointer) const noexcept;

    std::size_t block_size() const noexcept;
    std::size_t block_alignment() const noexcept;
    std::size_t capacity() const noexcept;
    std::size_t available_blocks() const noexcept;

private:
    struct State;

    static std::size_t effective_alignment(std::size_t alignment);
    static std::size_t align_up(std::size_t value, std::size_t alignment);
    static std::size_t calculate_stride(
        std::size_t block_size,
        std::size_t block_alignment
    );
    static void validate_configuration(
        std::size_t block_size,
        std::size_t block_alignment
    );
    static std::size_t& next_slot(State& state, std::size_t index) noexcept;
    static void* block_address(
        const State& state,
        std::size_t index
    ) noexcept;
    static std::optional<std::size_t> block_index(
        const State& state,
        const void* pointer
    ) noexcept;

    State* mState;
};

} // namespace hft::memory
