#pragma once

#include <cstddef>
#include <exception>
#include <utility>

#include "hft/memory/fixed_block_pool.hpp"

namespace hft::memory {

// Single-threaded typed pool. Objects must be released with destroy() before
// the pool is destroyed; the pool does not implicitly destroy live objects.
template <typename T>
class ObjectPool {
public:
    explicit ObjectPool(std::size_t capacity)
        : mPool(sizeof(T), alignof(T), capacity)
    {
    }

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(ObjectPool&&) = delete;

    template <typename... Args>
    T* create(Args&&... args)
    {
        void* storage = mPool.allocate();
        if (storage == nullptr) {
            return nullptr;
        }

        try {
            return ::new (storage) T(std::forward<Args>(args)...);
        } catch (...) {
            const bool released = mPool.deallocate(storage);
            if (!released) {
                std::terminate();
            }
            throw;
        }
    }

    bool destroy(T* object) noexcept
    {
        if (!mPool.owns_allocated_block(object)) {
            return false;
        }

        object->~T();
        return mPool.deallocate(object);
    }

    std::size_t capacity() const noexcept { return mPool.capacity(); }
    std::size_t available_objects() const noexcept
    {
        return mPool.available_blocks();
    }

private:
    FixedBlockPool mPool;
};

} // namespace hft::memory
