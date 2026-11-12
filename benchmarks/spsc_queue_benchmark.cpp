#include "hft/concurrency/spsc_queue.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>

int main()
{
    constexpr std::size_t operations = 1000000;
    hft::concurrency::SPSCQueue<std::uint64_t, 1024> queue;
    std::uint64_t value = 0;
    std::uint64_t output = 0;

    const auto start = std::chrono::steady_clock::now();
    for (std::size_t index = 0; index < operations; ++index) {
        while (!queue.try_push(value)) {
        }
        while (!queue.try_pop(output)) {
        }
        ++value;
    }
    const auto elapsed = std::chrono::steady_clock::now() - start;
    const auto nanos =
        std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();

    std::cout << "benchmark=spsc_queue operations=" << operations
              << " elapsed_ns=" << nanos
              << " operations_per_second="
              << (operations * 1'000'000'000ULL /
                  static_cast<unsigned long long>(nanos))
              << '\n';
}
