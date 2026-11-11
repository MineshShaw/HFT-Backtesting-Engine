#include "hft/platform/thread_affinity.hpp"

#include <gtest/gtest.h>

#ifdef __linux__

#include <cerrno>
#include <pthread.h>
#include <sched.h>

TEST(ThreadAffinityTest, PinsCurrentThreadToAnAllowedCpu)
{
    cpu_set_t allowed;
    CPU_ZERO(&allowed);
    ASSERT_EQ(sched_getaffinity(0, sizeof(allowed), &allowed), 0);

    unsigned int cpu_id = 0;
    while (cpu_id < CPU_SETSIZE && !CPU_ISSET(cpu_id, &allowed)) {
        ++cpu_id;
    }
    ASSERT_LT(cpu_id, CPU_SETSIZE);

    const auto result =
        hft::platform::pin_current_thread_to_cpu(cpu_id);

    ASSERT_TRUE(result.succeeded());
    EXPECT_EQ(result.error, hft::platform::ThreadAffinityError::None);
    EXPECT_EQ(result.system_error, 0);

    cpu_set_t current;
    CPU_ZERO(&current);
    ASSERT_EQ(
        pthread_getaffinity_np(
            pthread_self(),
            sizeof(current),
            &current
        ),
        0
    );
    EXPECT_TRUE(CPU_ISSET(cpu_id, &current));
}

TEST(ThreadAffinityTest, RejectsAnInvalidCpuId)
{
    const auto result =
        hft::platform::pin_current_thread_to_cpu(CPU_SETSIZE);

    EXPECT_FALSE(result.succeeded());
    EXPECT_EQ(result.error, hft::platform::ThreadAffinityError::InvalidCpu);
    EXPECT_EQ(result.system_error, EINVAL);
}

#else

TEST(ThreadAffinityTest, ReportsUnsupportedPlatform)
{
    const auto result = hft::platform::pin_current_thread_to_cpu(0);

    EXPECT_FALSE(result.succeeded());
    EXPECT_EQ(
        result.error,
        hft::platform::ThreadAffinityError::UnsupportedPlatform
    );
}

#endif
