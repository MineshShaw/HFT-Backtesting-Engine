#include "hft/memory/fixed_block_pool.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

int main()
{
    constexpr std::size_t capacity = 1024;
    constexpr std::size_t operations = 1000000;
    hft::memory::FixedBlockPool pool(sizeof(std::uint64_t), alignof(std::uint64_t), capacity);
    std::vector<void*> blocks;
    blocks.reserve(capacity);

    const auto start = std::chrono::steady_clock::now();
    for (std::size_t operation = 0; operation < operations; ++operation) {
        void* block = nullptr;
        while (block == nullptr) {
            block = pool.allocate();
        }
        blocks.push_back(block);
        if (blocks.size() == capacity) {
            for (void* value : blocks) {
                pool.deallocate(value);
            }
            blocks.clear();
        }
    }
    for (void* value : blocks) {
        pool.deallocate(value);
    }
    const auto elapsed = std::chrono::steady_clock::now() - start;
    const auto nanos =
        std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();

    std::cout << "benchmark=memory_pool operations=" << operations
              << " elapsed_ns=" << nanos
              << " operations_per_second="
              << (operations * 1'000'000'000ULL /
                  static_cast<unsigned long long>(nanos))
              << '\n';
}
