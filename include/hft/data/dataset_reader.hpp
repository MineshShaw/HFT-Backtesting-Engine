#pragma once

#include <cstddef>
#include <string>
#include <utility>

#include "hft/data/tick.hpp"
#include "hft/data/csv_parser.hpp"
#include "hft/data/mapped_file.hpp"

namespace hft::data {

struct IngestionStats {
    std::size_t lines_read{};
    std::size_t csv_records_accepted{};
    std::size_t csv_records_rejected{};
    std::size_t ticks_emitted{};
    std::size_t tick_conversion_rejected{};
};

class DatasetReader {
public:
    explicit DatasetReader(const std::string& file_path);

    DatasetReader(const DatasetReader&) = delete;
    DatasetReader& operator=(const DatasetReader&) = delete;

    DatasetReader(DatasetReader&& other) noexcept;
    DatasetReader& operator=(DatasetReader&& other) noexcept;

    bool is_valid() const;
    bool next(Tick& output);

    const IngestionStats& stats() const;

private:
    MappedFile mMappedFile;
    CsvParser mParser;
    IngestionStats mStats;
};

}