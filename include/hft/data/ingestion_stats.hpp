
#pragma once

#include <cstddef>

namespace hft::data {

struct IngestionStats {
    std::size_t lines_read{};
    std::size_t csv_records_accepted{};
    std::size_t csv_records_rejected{};
    std::size_t ticks_emitted{};
    std::size_t tick_conversion_rejected{};
};

}