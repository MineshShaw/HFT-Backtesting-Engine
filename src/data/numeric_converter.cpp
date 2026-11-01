#include "hft/data/numeric_converter.hpp"

#include <charconv>
#include <system_error>

namespace hft::data {

bool parse_double(std::string_view input, double& output)
{
    double temp;
    auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), temp);
    if (ec == std::errc{} && ptr == input.data() + input.size()) {
        output = temp;
        return true;
    }
    return false;
}

bool parse_integer(std::string_view input, int64_t& output)
{
    int64_t temp;
    auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), temp);
    if (ec == std::errc{} && ptr == input.data() + input.size()) {
        output = temp;
        return true;
    }
    return false;
}

} // namespace hft::data