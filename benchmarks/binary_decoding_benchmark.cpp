#include "hft/data/binary_parser.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

namespace {

void write_u16(
    std::vector<std::byte>& data,
    std::size_t offset,
    std::uint16_t value
)
{
    data[offset] = static_cast<std::byte>(value & 0xFFU);
    data[offset + 1] = static_cast<std::byte>(value >> 8);
}

void write_u32(
    std::vector<std::byte>& data,
    std::size_t offset,
    std::uint32_t value
)
{
    for (std::size_t index = 0; index < 4; ++index) {
        data[offset + index] =
            static_cast<std::byte>((value >> (index * 8)) & 0xFFU);
    }
}

void write_u64(
    std::vector<std::byte>& data,
    std::size_t offset,
    std::uint64_t value
)
{
    for (std::size_t index = 0; index < 8; ++index) {
        data[offset + index] =
            static_cast<std::byte>((value >> (index * 8)) & 0xFFU);
    }
}

} // namespace

int main()
{
    constexpr std::size_t records = 1000000;
    constexpr std::size_t header_size = 54;
    constexpr std::size_t record_size = 24;
    std::vector<std::byte> file(header_size + record_size);
    const std::array<std::uint8_t, 8> magic{
        'H', 'F', 'T', 'B', 'D', 'A', 'T', '\0'
    };
    for (std::size_t index = 0; index < magic.size(); ++index) {
        file[index] = static_cast<std::byte>(magic[index]);
    }
    write_u16(file, 8, 1);
    write_u16(file, 10, 0);
    write_u32(file, 12, header_size);
    write_u32(file, 16, record_size);
    write_u64(file, 24, 1);
    write_u32(file, 32, 1);
    write_u32(file, 36, 6);
    write_u32(file, 40, 100);
    write_u32(file, 44, 10);
    write_u16(file, 48, 4);
    file[50] = std::byte{'A'};
    file[51] = std::byte{'A'};
    file[52] = std::byte{'P'};
    file[53] = std::byte{'L'};
    write_u64(file, 54, 1);
    write_u64(file, 62, 12345);
    write_u16(file, 70, 0);
    write_u32(file, 74, 25);

    hft::data::BinaryHeader header{};
    hft::data::BinaryHeaderError header_error{};
    if (!hft::data::decodeBinaryHeader(file, header, header_error)) {
        return 1;
    }

    const auto record = std::span<const std::byte>(
        file.data() + header_size,
        record_size
    );
    hft::data::BinaryRecord decoded{};
    hft::data::BinaryRecordError record_error{};
    std::size_t decoded_count = 0;
    const auto start = std::chrono::steady_clock::now();
    for (std::size_t index = 0; index < records; ++index) {
        if (hft::data::decodeBinaryRecord(
                record,
                header,
                decoded,
                record_error
            )) {
            ++decoded_count;
        }
    }
    const auto elapsed = std::chrono::steady_clock::now() - start;
    const auto nanos =
        std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();

    std::cout << "benchmark=binary_decoding records=" << decoded_count
              << " elapsed_ns=" << nanos
              << " records_per_second="
              << (decoded_count * 1'000'000'000ULL /
                  static_cast<unsigned long long>(nanos))
              << '\n';
}
