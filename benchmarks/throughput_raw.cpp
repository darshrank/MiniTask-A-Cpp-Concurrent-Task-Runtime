#include "ThreadPool.hpp"

#include <chrono>
#include <iostream>

int main() {

    constexpr std::size_t NUM_WORKERS = 8;
    constexpr std::size_t NUM_TASKS = 1'000'000;

    ThreadPool pool(NUM_WORKERS, 4096);

    auto start = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < NUM_TASKS; ++i) {

        pool.submitDetached(
            TaskPriority::HIGH,
            [] {
                // intentionally empty
            });
    }

    pool.wait();

    auto end = std::chrono::steady_clock::now();

    double elapsed =
        std::chrono::duration<double>(end - start).count();

    double throughput =
        static_cast<double>(NUM_TASKS) / elapsed;

    std::cout << "\n========== Raw Throughput Benchmark ==========\n";

    std::cout << "Workers      : "
              << NUM_WORKERS << '\n';

    std::cout << "Tasks        : "
              << NUM_TASKS << '\n';

    std::cout << "Elapsed Time : "
              << elapsed << " s\n";

    std::cout << "Throughput   : "
              << throughput << " tasks/sec\n";

    std::cout << "Submitted    : "
              << pool.submittedTasks() << '\n';

    std::cout << "Completed    : "
              << pool.completedTasks() << '\n';

    std::cout << "Queued       : "
              << pool.queuedTasks() << '\n';
}