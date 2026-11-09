#pragma once

#include <string>

#include "hft/data/tick.hpp"
#include "hft/data/csv_parser.hpp"
#include "hft/data/mapped_file.hpp"
#include "hft/data/ingestion_stats.hpp"

namespace hft::data {

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