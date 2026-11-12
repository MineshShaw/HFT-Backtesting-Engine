#include "hft/data/csv_parser.hpp"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <string>

int main()
{
    constexpr std::size_t records = 100000;
    std::string input;
    input.reserve(records * 32);
    for (std::size_t index = 0; index < records; ++index) {
        input += std::to_string(index);
        input += ",AAPL,123.45,10\n";
    }

    hft::data::CsvParser parser(input);
    hft::data::TickView tick;
    std::size_t parsed = 0;
    const auto start = std::chrono::steady_clock::now();
    while (parser.next(tick) == hft::data::CsvParseResult::ValidRecord) {
        ++parsed;
    }
    const auto elapsed = std::chrono::steady_clock::now() - start;
    const auto nanos =
        std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();

    std::cout << "benchmark=csv_parsing records=" << parsed
              << " bytes=" << input.size()
              << " elapsed_ns=" << nanos
              << " records_per_second="
              << (parsed * 1'000'000'000ULL /
                  static_cast<unsigned long long>(nanos))
              << '\n';
}
