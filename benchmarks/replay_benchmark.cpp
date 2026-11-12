#include "hft/replay/event_loop.hpp"

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

int main()
{
    constexpr std::size_t records = 100000;
    const auto path =
        std::filesystem::temp_directory_path() / "hft_replay_benchmark.csv";
    {
        std::ofstream file(path);
        for (std::size_t index = 0; index < records; ++index) {
            file << index << ",AAPL,123.45,10\n";
        }
    }

    auto source =
        std::make_shared<hft::replay::CsvDataSource>(path.string());
    std::size_t consumed = 0;
    hft::replay::EventLoop loop(
        hft::replay::ReplaySource::from_owned(std::move(source)),
        [&](const hft::data::Tick&) {
            ++consumed;
        }
    );

    const auto start = std::chrono::steady_clock::now();
    if (!loop.start()) {
        std::filesystem::remove(path);
        return 1;
    }
    loop.join();
    const auto elapsed = std::chrono::steady_clock::now() - start;
    const auto nanos =
        std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
    std::filesystem::remove(path);

    std::cout << "benchmark=end_to_end_replay records=" << consumed
              << " elapsed_ns=" << nanos
              << " records_per_second="
              << (consumed * 1'000'000'000ULL /
                  static_cast<unsigned long long>(nanos))
              << '\n';
}
