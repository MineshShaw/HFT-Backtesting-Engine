#pragma once

#include <cstdint>

namespace hft::platform {

enum class ThreadAffinityError {
    None,
    UnsupportedPlatform,
    InvalidCpu,
    PermissionDenied,
    SystemError
};

struct ThreadAffinityResult {
    ThreadAffinityError error{ThreadAffinityError::None};
    int system_error{};

    bool succeeded() const noexcept
    {
        return error == ThreadAffinityError::None;
    }
};

// Performance-tuning utility only; scheduling affinity is not required for
// correctness and does not guarantee a performance improvement.
ThreadAffinityResult pin_current_thread_to_cpu(std::uint32_t cpu_id) noexcept;

} // namespace hft::platform
