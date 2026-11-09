#include "hft/data/mapped_file.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdint>
#include <limits>

namespace hft::data
{
MappedFile::MappedFile(const std::string& file_path)
    : mFileDescriptor(-1),
      mMappedData(nullptr),
      mMappedSize(0),
      mError(MappedFileError::None)
{
    mFileDescriptor = open(file_path.c_str(), O_RDONLY);
    if (mFileDescriptor == -1) {
        mError = MappedFileError::OpenFailed;
        return;
    }

    struct stat sb;
    if (fstat(mFileDescriptor, &sb) == -1) {
        close(mFileDescriptor);
        mFileDescriptor = -1;
        mError = MappedFileError::StatFailed;
        return;
    }

    if (sb.st_size < 0) {
        close(mFileDescriptor);
        mFileDescriptor = -1;
        mError = MappedFileError::StatFailed;
        return;
    }

    if (static_cast<std::uintmax_t>(sb.st_size) >
        std::numeric_limits<std::size_t>::max()) {
        close(mFileDescriptor);
        mFileDescriptor = -1;
        mError = MappedFileError::FileTooLarge;
        return;
    }

    mMappedSize = static_cast<std::size_t>(sb.st_size);

    if (mMappedSize == 0) {
        return;
    }

    mMappedData = mmap(nullptr, mMappedSize, PROT_READ, MAP_PRIVATE, mFileDescriptor, 0);
    if (mMappedData == MAP_FAILED) {
        close(mFileDescriptor);
        mFileDescriptor = -1;
        mMappedData = nullptr;
        mMappedSize = 0;
        mError = MappedFileError::MapFailed;
        return;
    }
}

MappedFile::MappedFile(MappedFile&& other) noexcept
    : mFileDescriptor(other.mFileDescriptor),
      mMappedData(other.mMappedData),
      mMappedSize(other.mMappedSize),
      mError(other.mError)
{
    other.mFileDescriptor = -1;
    other.mMappedData = nullptr;
    other.mMappedSize = 0;
    other.mError = MappedFileError::MovedFrom;
}

MappedFile::~MappedFile()
{
    if (mMappedData != nullptr) {
        munmap(mMappedData, mMappedSize);
    }
    if (mFileDescriptor != -1) {
        close(mFileDescriptor);
    }
}

MappedFile& MappedFile::operator=(MappedFile&& other) noexcept
{
    if (this != &other) {
        if (mMappedData != nullptr) {
            munmap(mMappedData, mMappedSize);
        }
        if (mFileDescriptor != -1) {
            close(mFileDescriptor);
        }

        mFileDescriptor = other.mFileDescriptor;
        mMappedData = other.mMappedData;
        mMappedSize = other.mMappedSize;
        mError = other.mError;

        other.mFileDescriptor = -1;
        other.mMappedData = nullptr;
        other.mMappedSize = 0;
        other.mError = MappedFileError::MovedFrom;
    }
    return *this;
}

const void* MappedFile::data() const
{
    return mMappedData;
}

std::size_t MappedFile::size() const
{
    return mMappedSize;
}

bool MappedFile::is_valid() const
{
    return mFileDescriptor != -1;
}

MappedFileError MappedFile::error() const
{
    return mError;
}
} // namespace hft::data