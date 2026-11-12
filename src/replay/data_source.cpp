#include "hft/replay/data_source.hpp"

namespace hft::replay {

CsvDataSource::CsvDataSource(const std::string& file_path)
    : mReader(file_path)
{
}

bool CsvDataSource::is_valid() const
{
    return mReader.is_valid();
}

ReadStatus CsvDataSource::next(data::Tick& output)
{
    return mReader.next(output)
               ? ReadStatus::TickAvailable
               : ReadStatus::EndOfFile;
}

BinaryDataSource::BinaryDataSource(
    const std::string& file_path,
    data::BinaryCorruptionPolicy policy
)
    : mReader(file_path, policy)
{
}

bool BinaryDataSource::is_valid() const
{
    return mReader.is_valid();
}

ReadStatus BinaryDataSource::next(data::Tick& output)
{
    switch (mReader.next(output)) {
        case data::BinaryReadStatus::TickAvailable:
            return ReadStatus::TickAvailable;
        case data::BinaryReadStatus::EndOfFile:
            return ReadStatus::EndOfFile;
        case data::BinaryReadStatus::Error:
            return ReadStatus::Error;
    }

    return ReadStatus::Error;
}

ReplaySource::ReplaySource(
    std::shared_ptr<void> owner,
    ValidFunction valid,
    NextFunction next
)
    : mOwner(std::move(owner)),
      mValid(valid),
      mNext(next)
{
}

bool ReplaySource::is_valid() const
{
    return mValid(mOwner.get());
}

ReadStatus ReplaySource::next(data::Tick& output)
{
    return mNext(mOwner.get(), output);
}

} // namespace hft::replay
