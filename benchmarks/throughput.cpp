#include "ThreadPool.hpp"

#include <chrono>
#include <future>
#include <iostream>
#include <vector>

int main() {
    constexpr std::size_t NUM_WORKERS = 8;
    constexpr std::size_t NUM_TASKS = 1'000'000;

    ThreadPool pool(NUM_WORKERS, NUM_TASKS);

    std::vector<std::future<void>> futures;
    futures.reserve(NUM_TASKS);

    auto start = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < NUM_TASKS; ++i) {
        futures.push_back(
            pool.submit(TaskPriority::HIGH, [] {
                // Intentionally empty
            })
        );
    }

    for (auto& future : futures) {
        future.get();
    }

    pool.wait();

    auto end = std::chrono::steady_clock::now();

    double elapsedSeconds =
        std::chrono::duration<double>(end - start).count();

    double throughput =
        static_cast<double>(NUM_TASKS) / elapsedSeconds;

    std::cout << "\n========== Throughput Benchmark ==========\n";

    std::cout << "Workers      : "
              << NUM_WORKERS
              << '\n';

    std::cout << "Tasks        : "
              << NUM_TASKS
              << '\n';

    std::cout << "Elapsed Time : "
              << elapsedSeconds
              << " s\n";

    std::cout << "Throughput   : "
              << throughput
              << " tasks/sec\n";

    std::cout << "Submitted    : "
              << pool.submittedTasks()
              << '\n';

    std::cout << "Completed    : "
              << pool.completedTasks()
              << '\n';

    std::cout << "Queued       : "
              << pool.queuedTasks()
              << '\n';
}