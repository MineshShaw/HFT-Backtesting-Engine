#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "hft/data/binary_format.hpp"
#include "hft/data/binary_parser.hpp"
#include "hft/data/mapped_file.hpp"
#include "hft/data/tick.hpp"

namespace hft::data {

enum class BinaryCorruptionPolicy {
    StopOnCorruption,
    SkipCorruptRecords
};

enum class BinaryReadStatus {
    TickAvailable,
    EndOfFile,
    Error
};

enum class BinaryErrorStage {
    Mapping,
    Header,
    SymbolTable,
    Record
};

struct BinaryDatasetError {
    BinaryErrorStage stage{};
    std::optional<MappedFileError> mapped_file_error;
    std::optional<BinaryHeaderError> header_error;
    std::optional<BinaryRecordError> record_error;
    std::uint64_t record_index{};
    std::size_t byte_offset{};
};

struct BinaryIngestionStats {
    std::uint64_t records_seen{};
    std::uint64_t records_emitted{};
    std::uint64_t records_skipped{};
};

class BinaryDatasetReader {
public:
    explicit BinaryDatasetReader(
        const std::string& file_path,
        BinaryCorruptionPolicy policy = BinaryCorruptionPolicy::StopOnCorruption
    );

    bool is_valid() const;

    // Tick::symbol refers into this reader's mapped file and must not outlive it.
    BinaryReadStatus next(Tick& output);

    const BinaryIngestionStats& stats() const;
    const std::optional<BinaryDatasetError>& initialization_error() const;
    const std::optional<BinaryDatasetError>& last_error() const;
    const std::optional<BinaryDatasetError>& last_corruption() const;

private:
    MappedFile mMappedFile;
    BinaryHeader mHeader{};
    std::vector<std::string_view> mSymbols;
    BinaryCorruptionPolicy mPolicy;
    BinaryIngestionStats mStats{};
    std::optional<BinaryDatasetError> mInitializationError;
    std::optional<BinaryDatasetError> mLastError;
    std::optional<BinaryDatasetError> mLastCorruption;
    std::uint64_t mNextRecordIndex{};
    bool mStopped{};
};

} // namespace hft::data