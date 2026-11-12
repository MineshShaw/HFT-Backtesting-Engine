#pragma once

#include <concepts>
#include <memory>
#include <string>

#include "hft/data/binary_dataset_reader.hpp"
#include "hft/data/dataset_reader.hpp"

namespace hft::replay {

enum class ReadStatus {
    TickAvailable,
    EndOfFile,
    Error
};

template <typename Source>
concept ReplayDataSource = requires(Source& source, data::Tick& tick) {
    { source.is_valid() } -> std::convertible_to<bool>;
    { source.next(tick) } -> std::same_as<ReadStatus>;
};

class CsvDataSource {
public:
    explicit CsvDataSource(const std::string& file_path);

    bool is_valid() const;
    ReadStatus next(data::Tick& output);

private:
    data::DatasetReader mReader;
};

class BinaryDataSource {
public:
    explicit BinaryDataSource(
        const std::string& file_path,
        data::BinaryCorruptionPolicy policy =
            data::BinaryCorruptionPolicy::StopOnCorruption
    );

    bool is_valid() const;
    ReadStatus next(data::Tick& output);

private:
    data::BinaryDatasetReader mReader;
};

static_assert(ReplayDataSource<CsvDataSource>);
static_assert(ReplayDataSource<BinaryDataSource>);

class ReplaySource {
public:
    using NextFunction = ReadStatus (*)(void*, data::Tick&);
    using ValidFunction = bool (*)(const void*);

    ReplaySource() = delete;

    template <ReplayDataSource Source>
    static ReplaySource from_owned(std::shared_ptr<Source> source)
    {
        return ReplaySource(
            std::move(source),
            [](const void* value) {
                return static_cast<const Source*>(value)->is_valid();
            },
            [](void* value, data::Tick& tick) {
                return static_cast<Source*>(value)->next(tick);
            }
        );
    }

    bool is_valid() const;
    ReadStatus next(data::Tick& output);

private:
    ReplaySource(
        std::shared_ptr<void> owner,
        ValidFunction valid,
        NextFunction next
    );

    std::shared_ptr<void> mOwner;
    ValidFunction mValid;
    NextFunction mNext;
};

} // namespace hft::replay
