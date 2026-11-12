# HFT Backtesting Engine

This repository contains the Week 1 and Week 2 foundation of a historical
market-data replay engine. It is not a live-exchange gateway and does not
claim production-HFT readiness.

## Architecture

```text
CSV or binary file
        |
        v
MappedFile + reader/parser
        |
        v
ReplayDataSource adapter
        |
        v
producer thread -> SPSCQueue<Tick> -> consumer callback
```

The event loop owns an `std::shared_ptr` to its source. The producer does not
destroy the source at EOF. Both threads are joined, and queued ticks are
drained, before the event loop releases the source and queue. This is required
because `Tick::symbol` is a non-owning `std::string_view` into mapped file
memory.

## Build and test

Requirements:

- C++20 compiler
- CMake 3.20 or newer
- GoogleTest
- pthreads on Linux

```bash
cmake -S . -B build
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

Strict warnings are enabled for project targets: `-Wall -Wextra -Wpedantic
-Werror` on GCC/Clang.

## Data formats

### CSV

CSV records contain four fields:

```text
timestamp,symbol,price,size
```

The parser is zero-copy: fields are `std::string_view`s into mapped file
memory. It supports whitespace trimming, CRLF endings, final lines without a
newline, and skips malformed records while recording ingestion statistics.
Quoted CSV fields and escaping are not implemented.

### Binary

Binary files use version 1.0, little-endian encoding, and a 48-byte fixed
header followed by a length-prefixed symbol table and 24-byte records.

Record offsets:

| Field | Offset | Size |
|---|---:|---:|
| timestamp (`int64_t`) | 0 | 8 |
| fixed-point price (`int64_t`) | 8 | 8 |
| symbol ID (`uint16_t`) | 16 | 2 |
| reserved | 18 | 2 |
| quantity (`uint32_t`) | 20 | 4 |

Price and quantity are converted by dividing by the nonzero header scales.
Binary decoding reads fields explicitly; mapped bytes are never reinterpreted
as C++ structs. Symbol views point into the mapping and must not outlive the
owning reader.

## Memory pool

`FixedBlockPool` owns one aligned backing allocation and provides O(1)
single-threaded fixed-block allocation/deallocation. It rejects null, foreign,
misaligned, and repeated deallocation. `ObjectPool<T>` constructs objects in
place and requires callers to destroy live objects before pool destruction.
Neither pool is thread-safe or a general-purpose allocator.

## SPSC queue

`SPSCQueue<T, Capacity>` supports general C++ object types, including
non-trivial and move-only types. It uses preallocated storage and exactly one
producer and one consumer. Multiple producers or consumers are unsupported.

The producer constructs a value and release-publishes the write position. The
consumer acquire-loads that position before reading, destroys the value, and
release-publishes the read position. The producer acquire-loads the read
position before reusing a slot. These acquire/release pairs establish the
happens-before relationships for object construction, observation, destruction,
and slot reuse. The indices are separated on 64-byte cache-line boundaries;
this is a common Linux x86 cache-line assumption, not a correctness
requirement.

The queue is non-blocking: full and empty operations return `false`. The
caller owns retry/backpressure policy.

## Thread affinity

`pin_current_thread_to_cpu()` is an optional Linux scheduling-tuning utility
using pthread affinity APIs. CPU IDs are supplied by the caller and are not
hardcoded. Affinity is not required for correctness, and this project makes no
claim that it improves performance without measurement.

## Event-loop lifecycle

The common source contract is checked with a C++20 concept requiring
`is_valid()` and `next(Tick&)`, returning the common `ReadStatus`. CSV and
binary readers retain their separate error types and statistics.

The event loop starts one producer and one consumer. The producer retries when
the queue is full using `std::this_thread::yield()`, never discarding a tick.
EOF and errors are stored separately from queued data. Consumer callback
exceptions request shutdown and are reported after both threads join.
Requested shutdown stops production, then drains already queued ticks before
completion.

`events_produced` counts ticks successfully published into the queue,
`events_consumed` counts callbacks that returned normally, `source_errors`
counts initialization and runtime source failures, and `consumer_errors`
counts callback exceptions. Statistics are final after `join()` returns.

## Benchmarks

Standalone benchmark executables are built but are not CTest correctness
tests:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel 2
./build-release/csv_parsing_benchmark
./build-release/binary_decoding_benchmark
./build-release/memory_pool_benchmark
./build-release/spsc_queue_benchmark
./build-release/replay_benchmark
```

The benchmarks report elapsed nanoseconds and operations or records per
second. CSV parsing uses 100,000 generated records; binary decoding uses
1,000,000 repeated decodes of one valid 24-byte record; the pool and queue
benchmarks use 1,000,000 operations; end-to-end replay uses 100,000 generated
CSV records. Results depend on compiler, optimization level, CPU frequency,
OS scheduling, affinity, and system load. Benchmark output must be recorded
from the target machine; no performance target is assumed.

## Current scope and next steps

Week 1 and Week 2 provide mapped data access, CSV and binary readers, parsing,
statistics, a memory pool, an SPSC queue, optional Linux affinity, and a
threaded replay pipeline. Week 3 may add an order book, matching engine, and
strategy interface; those components are intentionally not included here.
