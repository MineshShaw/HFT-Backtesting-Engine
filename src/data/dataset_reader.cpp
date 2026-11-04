#include "hft/data/dataset_reader.hpp"

#include "hft/data/tick_converter.hpp"

namespace hft::data {

DatasetReader::DatasetReader(const std::string& file_path)
    : mMappedFile(file_path),
      mParser(mMappedFile.is_valid()
                  ? std::string_view(
                        static_cast<const char*>(mMappedFile.data()),
                        mMappedFile.size())
                  : std::string_view{}),
      mStats{}
{}

bool DatasetReader::is_valid() const
{
    return mMappedFile.is_valid();
}

const IngestionStats& DatasetReader::stats() const
{
    return mStats;
}

bool DatasetReader::next(Tick& output)
{
    while (true) {
        TickView view;

        const auto result = mParser.next(view);

        if (result == CsvParseResult::EndOfInput) {
            return false;
        }

        ++mStats.lines_read;

        if (result == CsvParseResult::InvalidRecord) {
            ++mStats.csv_records_rejected;
            continue;
        }

        ++mStats.csv_records_accepted;

        if (convert_tick(view, output)) {
            ++mStats.ticks_emitted;
            return true;
        }

        ++mStats.tick_conversion_rejected;
    }
}

DatasetReader::DatasetReader(DatasetReader&& other) noexcept
    : mMappedFile(std::move(other.mMappedFile)),
      mParser(std::move(other.mParser)),
      mStats(other.mStats)
{
    other.mParser = CsvParser{};
    other.mStats = {};
}

DatasetReader& DatasetReader::operator=(DatasetReader&& other) noexcept
{
    if (this != &other) {
        mParser = CsvParser{};
        mMappedFile = std::move(other.mMappedFile);
        mParser = std::move(other.mParser);
        mStats = other.mStats;

        other.mParser = CsvParser{};
        other.mStats = {};
    }

    return *this;
}

} // namespace hft::data