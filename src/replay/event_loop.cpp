#include "hft/replay/event_loop.hpp"

#include <atomic>
#include <exception>
#include <memory>
#include <sstream>
#include <thread>

#include "hft/concurrency/spsc_queue.hpp"

namespace hft::replay {

struct EventLoop::QueueState {
    concurrency::SPSCQueue<data::Tick, 1024> queue;
};

EventLoop::EventLoop(ReplaySource source, Consumer consumer)
    : mSource(std::move(source)),
      mConsumer(std::move(consumer)),
      mQueue(new QueueState()),
      mShutdownRequested(false),
      mProducerDone(false)
{
}

EventLoop::~EventLoop()
{
    request_shutdown();
    join();
    delete mQueue;
}

bool EventLoop::start()
{
    if (mStarted) {
        return false;
    }
    mStarted = true;

    if (!mSource.is_valid()) {
        set_error("Replay source initialization failed", ShutdownReason::SourceError);
        mProducerDone.store(true, std::memory_order_release);
        return false;
    }

    mProducer = std::thread(&EventLoop::producer_main, this);
    mConsumerThread = std::thread(&EventLoop::consumer_main, this);
    return true;
}

void EventLoop::request_shutdown()
{
    const bool was_requested = mShutdownRequested.exchange(
        true,
        std::memory_order_acq_rel
    );
    if (!was_requested) {
        std::scoped_lock lock(mStateMutex);
        if (mStats.shutdown_reason == ShutdownReason::None) {
            mStats.shutdown_reason = ShutdownReason::Requested;
        }
    }
}

void EventLoop::join()
{
    if (mProducer.joinable()) {
        mProducer.join();
    }
    if (mConsumerThread.joinable()) {
        mConsumerThread.join();
    }
}

const EventLoopStats& EventLoop::stats() const
{
    return mStats;
}

const std::optional<std::string>& EventLoop::error() const
{
    return mError;
}

void EventLoop::set_error(std::string message, ShutdownReason reason)
{
    std::scoped_lock lock(mStateMutex);
    if (!mError.has_value()) {
        mError = std::move(message);
    }
    if (mStats.shutdown_reason != ShutdownReason::Requested) {
        mStats.shutdown_reason = reason;
    }
}

void EventLoop::producer_main()
{
    try {
        while (!mShutdownRequested.load(std::memory_order_acquire)) {
            data::Tick tick{};
            const ReadStatus status = mSource.next(tick);

            if (status == ReadStatus::EndOfFile) {
                std::scoped_lock lock(mStateMutex);
                if (mStats.shutdown_reason == ShutdownReason::None) {
                    mStats.shutdown_reason = ShutdownReason::Completed;
                }
                break;
            }

            if (status == ReadStatus::Error) {
                {
                    std::scoped_lock lock(mStateMutex);
                    ++mStats.source_errors;
                }
                set_error("Replay source returned an error", ShutdownReason::SourceError);
                break;
            }

            while (!mQueue->queue.try_push(std::move(tick))) {
                if (mShutdownRequested.load(std::memory_order_acquire)) {
                    break;
                }
                std::this_thread::yield();
            }

            if (mShutdownRequested.load(std::memory_order_acquire)) {
                break;
            }

            std::scoped_lock lock(mStateMutex);
            ++mStats.events_produced;
        }
    } catch (const std::exception& exception) {
        set_error(exception.what(), ShutdownReason::SourceError);
        std::scoped_lock lock(mStateMutex);
        ++mStats.source_errors;
    } catch (...) {
        set_error("Unknown producer exception", ShutdownReason::SourceError);
        std::scoped_lock lock(mStateMutex);
        ++mStats.source_errors;
    }

    mProducerDone.store(true, std::memory_order_release);
}

void EventLoop::consumer_main()
{
    try {
        for (;;) {
            data::Tick tick{};
            if (mQueue->queue.try_pop(tick)) {
                mConsumer(tick);
                std::scoped_lock lock(mStateMutex);
                ++mStats.events_consumed;
                continue;
            }

            const bool producer_done = mProducerDone.load(
                std::memory_order_acquire
            );
            if (producer_done || mShutdownRequested.load(
                                    std::memory_order_acquire
                                )) {
                break;
            }

            std::this_thread::yield();
        }
    } catch (const std::exception& exception) {
        {
            std::scoped_lock lock(mStateMutex);
            ++mStats.consumer_errors;
        }
        set_error(exception.what(), ShutdownReason::ConsumerError);
        mShutdownRequested.store(true, std::memory_order_release);
    } catch (...) {
        {
            std::scoped_lock lock(mStateMutex);
            ++mStats.consumer_errors;
        }
        set_error("Unknown consumer exception", ShutdownReason::ConsumerError);
        mShutdownRequested.store(true, std::memory_order_release);
    }
}

} // namespace hft::replay
