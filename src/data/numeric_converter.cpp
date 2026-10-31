#include "hft/data/numeric_converter.hpp"

#include <charconv>
#include <system_error>

namespace hft::data {

bool parse_double(std::string_view input, double& output)
{
    auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), output);
    return ec == std::errc{} && ptr == input.data() + input.size();
}

bool parse_integer(std::string_view input, int64_t& output)
{
    auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), output);
    return ec == std::errc{} && ptr == input.data() + input.size();
}

} // namespace hft::data