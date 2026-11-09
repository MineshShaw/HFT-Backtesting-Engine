
#include <string>

#include "hft/data/tick.hpp"
#include "hft/data/ingestion_stats.hpp"

namespace hft::data {

class BinaryDatasetReader {
public:
    explicit BinaryDatasetReader(const std::string& file_path);

    bool is_valid() const;
    bool next(Tick& output);

    const IngestionStats& stats() const;
};

}