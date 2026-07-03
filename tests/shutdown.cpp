#include "ThreadPool.hpp"

#include <cassert>
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

int main() {

    ThreadPool pool(4, 100);

    constexpr int NUM_TASKS = 20;

    std::atomic<int> completed{0};

    // Submit long-running tasks
    for (int i = 0; i < NUM_TASKS; ++i) {
        pool.submit(TaskPriority::LOW, [&completed] {

            std::this_thread::sleep_for(
                std::chrono::milliseconds(100));

            completed.fetch_add(1, std::memory_order_relaxed);
        });
    }

    auto start = std::chrono::steady_clock::now();

    pool.shutdown();

    auto end = std::chrono::steady_clock::now();

    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start);

    // All submitted work should complete.
    assert(completed.load() == NUM_TASKS);

    assert(pool.submittedTasks() == NUM_TASKS);
    assert(pool.completedTasks() == NUM_TASKS);
    assert(pool.queuedTasks() == 0);

    // Calling shutdown() again should be harmless.
    pool.shutdown();

    // No new submissions allowed.
    bool exceptionThrown = false;

    try {

        pool.submit(TaskPriority::LOW, [] {
            return 42;
        });

    } catch (const std::runtime_error&) {

        exceptionThrown = true;
    }

    assert(exceptionThrown);

    std::cout << "===== Shutdown Test Passed =====\n";
    std::cout << "Elapsed Time: "
              << elapsed.count()
              << " ms\n";
}