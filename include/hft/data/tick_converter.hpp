
#pragma once

#include "hft/data/csv_parser.hpp"
#include "hft/data/tick.hpp"

namespace hft::data {

bool convert_tick(const TickView& input, Tick& output);

} // namespace hft::data
