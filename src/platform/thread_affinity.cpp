#include "hft/platform/thread_affinity.hpp"

#ifdef __linux__

#include <cerrno>
#include <pthread.h>
#include <sched.h>

namespace hft::platform {

namespace {

ThreadAffinityResult make_error(int error)
{
    ThreadAffinityResult result{};
    result.system_error = error;

    if (error == EINVAL) {
        result.error = ThreadAffinityError::InvalidCpu;
    } else if (error == EACCES || error == EPERM) {
        result.error = ThreadAffinityError::PermissionDenied;
    } else {
        result.error = ThreadAffinityError::SystemError;
    }

    return result;
}

} // namespace

ThreadAffinityResult pin_current_thread_to_cpu(std::uint32_t cpu_id) noexcept
{
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);

    if (cpu_id >= CPU_SETSIZE) {
        return make_error(EINVAL);
    }

    CPU_SET(cpu_id, &cpu_set);
    const int result = pthread_setaffinity_np(
        pthread_self(),
        sizeof(cpu_set),
        &cpu_set
    );

    if (result != 0) {
        return make_error(result);
    }

    return {};
}

} // namespace hft::platform

#else

namespace hft::platform {

ThreadAffinityResult pin_current_thread_to_cpu(std::uint32_t) noexcept
{
    return {ThreadAffinityError::UnsupportedPlatform, 0};
}

} // namespace hft::platform

#endif
