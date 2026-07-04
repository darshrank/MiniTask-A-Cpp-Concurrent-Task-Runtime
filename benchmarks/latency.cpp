#include "ThreadPool.hpp"

#include <algorithm>
#include <chrono>
#include <future>
#include <iostream>
#include <numeric>
#include <vector>

using Clock = std::chrono::steady_clock;
using Nanoseconds = std::chrono::nanoseconds;

int main() {

    constexpr std::size_t NUM_WORKERS = 8;
    constexpr std::size_t NUM_TASKS = 100000;

    ThreadPool pool(NUM_WORKERS, 1000);

    std::vector<long long> latencies;
    latencies.reserve(NUM_TASKS);

    for (std::size_t i = 0; i < NUM_TASKS; ++i) {

        auto submitTime = Clock::now();

        auto future = pool.submit(
            TaskPriority::HIGH,
            [submitTime]() {

                auto finishTime = Clock::now();

                return std::chrono::duration_cast<Nanoseconds>(
                           finishTime - submitTime)
                    .count();
            });

        latencies.push_back(future.get());
    }

    std::sort(latencies.begin(), latencies.end());

    long long total =
        std::accumulate(latencies.begin(),
                        latencies.end(),
                        0LL);

    double average =
        static_cast<double>(total) / latencies.size();

    auto percentile = [&](double p) -> long long {
        std::size_t idx =
            static_cast<std::size_t>(p * latencies.size());

        if (idx >= latencies.size())
            idx = latencies.size() - 1;

        return latencies[idx];
    };

    std::cout << "\n========== Latency Benchmark ==========\n";

    std::cout << "Workers : "
              << NUM_WORKERS
              << '\n';

    std::cout << "Tasks   : "
              << NUM_TASKS
              << '\n';

    std::cout << "Average : "
              << average
              << " ns\n";

    std::cout << "p50     : "
              << percentile(0.50)
              << " ns\n";

    std::cout << "p95     : "
              << percentile(0.95)
              << " ns\n";

    std::cout << "p99     : "
              << percentile(0.99)
              << " ns\n";

    std::cout << "Min     : "
              << latencies.front()
              << " ns\n";

    std::cout << "Max     : "
              << latencies.back()
              << " ns\n";

    return 0;
}