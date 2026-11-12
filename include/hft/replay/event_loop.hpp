#pragma once

#include <cstddef>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "hft/data/tick.hpp"
#include "hft/replay/data_source.hpp"

namespace hft::replay {

enum class ShutdownReason {
    None,
    Completed,
    SourceError,
    ConsumerError,
    Requested
};

struct EventLoopStats {
    std::size_t events_produced{};
    std::size_t events_consumed{};
    std::size_t source_errors{};
    std::size_t consumer_errors{};
    ShutdownReason shutdown_reason{ShutdownReason::None};
};

class EventLoop {
public:
    using Consumer = std::function<void(const data::Tick&)>;

    EventLoop(ReplaySource source, Consumer consumer);
    ~EventLoop();

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;
    EventLoop(EventLoop&&) = delete;
    EventLoop& operator=(EventLoop&&) = delete;

    bool start();
    void request_shutdown();
    void join();

    const EventLoopStats& stats() const;
    const std::optional<std::string>& error() const;

private:
    void producer_main();
    void consumer_main();
    void set_error(std::string message, ShutdownReason reason);

    ReplaySource mSource;
    Consumer mConsumer;
    struct QueueState;
    QueueState* mQueue;
    std::thread mProducer;
    std::thread mConsumerThread;
    std::atomic<bool> mShutdownRequested;
    std::atomic<bool> mProducerDone;
    EventLoopStats mStats{};
    std::optional<std::string> mError;
    mutable std::mutex mStateMutex;
    bool mStarted{};
};

} // namespace hft::replay
