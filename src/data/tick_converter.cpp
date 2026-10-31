#include "hft/data/tick_converter.hpp"
#include "hft/data/numeric_converter.hpp"

namespace hft::data {

bool convert_tick(const TickView& input, Tick& output) {
    Tick temp_tick;
    if (!parse_integer(input.timestamp, temp_tick.timestamp)) {
        return false;
    }
    temp_tick.symbol = input.symbol;

    if (!parse_double(input.price, temp_tick.price)) {
        return false;
    }
    if (!parse_double(input.size, temp_tick.size)) {
        return false;
    }
    
    output = temp_tick;
    return true;
}

}