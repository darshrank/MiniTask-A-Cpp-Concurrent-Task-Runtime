#include "ThreadPool.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <numeric>
#include <vector>

using Clock = std::chrono::steady_clock;
using namespace std::chrono;

int main() {

    constexpr std::size_t NUM_WORKERS = 8;
    constexpr std::size_t NUM_LOW_TASKS = 100000;
    constexpr std::size_t NUM_HIGH_TASKS = 10000;
    constexpr std::size_t QUEUE_SIZE = NUM_LOW_TASKS + NUM_HIGH_TASKS;

    ThreadPool pool(NUM_WORKERS, QUEUE_SIZE);

    std::vector<long long> highLatencies;
    highLatencies.reserve(NUM_HIGH_TASKS);

    std::mutex latencyMutex;

    std::atomic<std::size_t> completedHigh{0};
    std::atomic<std::size_t> completedLow{0};

    // Submit LOW priority tasks first to create a backlog.
    for (std::size_t i = 0; i < NUM_LOW_TASKS; ++i) {

        pool.submitDetached(
            TaskPriority::LOW,
            [&completedLow]() {

                volatile int x = 0;

                for (int j = 0; j < 100; ++j) {
                    x += j;
                }

                completedLow.fetch_add(
                    1,
                    std::memory_order_relaxed);
            });
    }

    // Submit HIGH priority tasks afterwards.
    for (std::size_t i = 0; i < NUM_HIGH_TASKS; ++i) {

        auto submitTime = Clock::now();

        pool.submitDetached(
            TaskPriority::HIGH,
            [&highLatencies,
             &latencyMutex,
             &completedHigh,
             submitTime]() {

                auto startTime = Clock::now();

                long long latency =
                    duration_cast<nanoseconds>(
                        startTime - submitTime).count();

                {
                    std::lock_guard<std::mutex> lock(latencyMutex);
                    highLatencies.push_back(latency);
                }

                volatile int x = 0;

                for (int j = 0; j < 100; ++j) {
                    x += j;
                }

                completedHigh.fetch_add(
                    1,
                    std::memory_order_relaxed);
            });
    }

    pool.wait();

    std::sort(
        highLatencies.begin(),
        highLatencies.end());

    double average =
        std::accumulate(
            highLatencies.begin(),
            highLatencies.end(),
            0.0)
        / highLatencies.size();

    auto p50 =
        highLatencies[highLatencies.size() * 50 / 100];

    auto p95 =
        highLatencies[highLatencies.size() * 95 / 100];

    auto p99 =
        highLatencies[highLatencies.size() * 99 / 100];

    std::cout << "\n========== Priority Benchmark ==========\n\n";

    std::cout << "Workers          : "
              << NUM_WORKERS << '\n';

    std::cout << "LOW Tasks        : "
              << NUM_LOW_TASKS << '\n';

    std::cout << "HIGH Tasks       : "
              << NUM_HIGH_TASKS << "\n\n";

    std::cout << "HIGH Task Scheduling Latency\n";
    std::cout << "----------------------------\n";

    std::cout << "Average : "
              << std::fixed << std::setprecision(1)
              << average << " ns\n";

    std::cout << "p50     : "
              << p50 << " ns\n";

    std::cout << "p95     : "
              << p95 << " ns\n";

    std::cout << "p99     : "
              << p99 << " ns\n\n";

    std::cout << "Completed HIGH : "
              << completedHigh.load() << '\n';

    std::cout << "Completed LOW  : "
              << completedLow.load() << '\n';

    return 0;
}