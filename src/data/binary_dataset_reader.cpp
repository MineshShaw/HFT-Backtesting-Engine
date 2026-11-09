#include "hft/data/binary_dataset_reader.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace hft::data {

namespace {

BinaryDatasetError makeMappingError(MappedFileError error)
{
    BinaryDatasetError result{};
    result.stage = BinaryErrorStage::Mapping;
    result.mapped_file_error = error;
    return result;
}

BinaryDatasetError makeHeaderError(BinaryHeaderError error)
{
    BinaryDatasetError result{};
    result.stage = BinaryErrorStage::Header;
    result.header_error = error;
    return result;
}

BinaryDatasetError makeSymbolTableError(BinaryHeaderError error)
{
    BinaryDatasetError result{};
    result.stage = BinaryErrorStage::SymbolTable;
    result.header_error = error;
    return result;
}

BinaryDatasetError makeRecordError(
    BinaryRecordError error,
    std::uint64_t record_index,
    std::size_t byte_offset
)
{
    BinaryDatasetError result{};
    result.stage = BinaryErrorStage::Record;
    result.record_error = error;
    result.record_index = record_index;
    result.byte_offset = byte_offset;
    return result;
}

} // namespace

BinaryDatasetReader::BinaryDatasetReader(
    const std::string& file_path,
    BinaryCorruptionPolicy policy
)
    : mMappedFile(file_path),
      mPolicy(policy)
{
    if (!mMappedFile.is_valid()) {
        mInitializationError = makeMappingError(mMappedFile.error());
        return;
    }

    const auto file = std::span<const std::byte>(
        static_cast<const std::byte*>(mMappedFile.data()),
        mMappedFile.size()
    );

    BinaryHeaderError header_error{};
    if (!decodeBinaryHeader(file, mHeader, header_error)) {
        mInitializationError = makeHeaderError(header_error);
        return;
    }

    if (!decodeSymbolTable(file, mSymbols, header_error)) {
        mInitializationError = makeSymbolTableError(header_error);
    }
}

bool BinaryDatasetReader::is_valid() const
{
    return !mInitializationError.has_value();
}

BinaryReadStatus BinaryDatasetReader::next(Tick& output)
{
    if (!is_valid()) {
        mLastError = mInitializationError;
        return BinaryReadStatus::Error;
    }

    if (mStopped) {
        return BinaryReadStatus::Error;
    }

    const auto file = std::span<const std::byte>(
        static_cast<const std::byte*>(mMappedFile.data()),
        mMappedFile.size()
    );

    while (mNextRecordIndex < mHeader.record_count) {
        const auto record_offset_u64 =
            static_cast<std::uint64_t>(mHeader.header_size) +
            mNextRecordIndex * static_cast<std::uint64_t>(mHeader.record_size);
        const auto record_offset = static_cast<std::size_t>(record_offset_u64);
        const auto record = file.subspan(record_offset, mHeader.record_size);

        ++mStats.records_seen;

        BinaryRecord decoded_record{};
        BinaryRecordError record_error{};
        if (!decodeBinaryRecord(record, mHeader, decoded_record, record_error)) {
            const auto corruption = makeRecordError(
                record_error,
                mNextRecordIndex,
                record_offset
            );
            mLastCorruption = corruption;
            ++mNextRecordIndex;

            if (mPolicy == BinaryCorruptionPolicy::SkipCorruptRecords) {
                ++mStats.records_skipped;
                continue;
            }

            mLastError = corruption;
            mStopped = true;
            return BinaryReadStatus::Error;
        }

        Tick decoded_tick{};
        decoded_tick.timestamp = decoded_record.timestamp;
        decoded_tick.symbol = mSymbols[decoded_record.symbol_id];
        decoded_tick.price = static_cast<double>(decoded_record.price) /
                             static_cast<double>(mHeader.price_scale);
        decoded_tick.size = static_cast<double>(decoded_record.quantity) /
                            static_cast<double>(mHeader.quantity_scale);

        output = decoded_tick;
        ++mNextRecordIndex;
        ++mStats.records_emitted;
        return BinaryReadStatus::TickAvailable;
    }

    return BinaryReadStatus::EndOfFile;
}

const BinaryIngestionStats& BinaryDatasetReader::stats() const
{
    return mStats;
}

const std::optional<BinaryDatasetError>&
BinaryDatasetReader::initialization_error() const
{
    return mInitializationError;
}

const std::optional<BinaryDatasetError>&
BinaryDatasetReader::last_error() const
{
    return mLastError;
}

const std::optional<BinaryDatasetError>&
BinaryDatasetReader::last_corruption() const
{
    return mLastCorruption;
}

} // namespace hft::data
