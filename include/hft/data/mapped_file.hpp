#pragma once

#include <cstddef>
#include <string>

namespace hft::data
{

enum class MappedFileError {
    None,
    OpenFailed,
    StatFailed,
    FileTooLarge,
    MapFailed,
    MovedFrom
};

class MappedFile {
public:
    MappedFile(const std::string& file_path);

    ~MappedFile();

    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;

    MappedFile(MappedFile&& other) noexcept;
    MappedFile& operator=(MappedFile&& other) noexcept;

    const void* data() const;

    std::size_t size() const;

    bool is_valid() const;

    MappedFileError error() const;

private:

    int mFileDescriptor;

    void* mMappedData;

    std::size_t mMappedSize;

    MappedFileError mError;
};

}