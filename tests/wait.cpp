#include "ThreadPool.hpp"

#include <cassert>
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

int main() {

    constexpr int NUM_TASKS = 20;

    ThreadPool pool(4, 100);

    std::atomic<int> finishedTasks{0};

    for (int i = 0; i < NUM_TASKS; ++i) {
        pool.submit(TaskPriority::LOW, [&finishedTasks] {

            std::this_thread::sleep_for(
                std::chrono::milliseconds(100));

            finishedTasks.fetch_add(
                1,
                std::memory_order_relaxed);

            return;
        });
    }

    auto start = std::chrono::steady_clock::now();

    pool.wait();

    auto end = std::chrono::steady_clock::now();

    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start);

    assert(finishedTasks.load() == NUM_TASKS);
    assert(pool.submittedTasks() == NUM_TASKS);
    assert(pool.completedTasks() == NUM_TASKS);
    assert(pool.queuedTasks() == 0);

    std::cout << "===== Wait Test Passed =====\n";
    std::cout << "Elapsed Time: "
              << elapsed.count()
              << " ms\n";
}