
#pragma once

#include <string_view>
#include <cstdint>

namespace hft::data {

bool parse_double(std::string_view input, double& output);

bool parse_integer(std::string_view input, int64_t& output);

} // namespace hft::data
