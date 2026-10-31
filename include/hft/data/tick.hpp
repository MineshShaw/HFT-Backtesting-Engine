
#pragma once

#include <cstdint>
#include <string_view>

namespace hft::data {

struct Tick {
    int64_t timestamp{};
    std::string_view symbol{};
    double price{};
    double size{};
};

} // namespace hft::data
