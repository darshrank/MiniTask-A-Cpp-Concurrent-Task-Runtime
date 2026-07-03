#include "ThreadPool.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

int main() {

    ThreadPool pool(4, 100);

    constexpr int NUM_HIGH = 40;
    constexpr int NUM_LOW  = 10;

    std::atomic<int> highCompleted{0};
    std::atomic<int> lowCompleted{0};

    // Submit HIGH tasks first.
    for (int i = 0; i < NUM_HIGH; ++i) {
        pool.submit(TaskPriority::HIGH,
            [&highCompleted] {

                std::this_thread::sleep_for(
                    std::chrono::milliseconds(20));

                highCompleted.fetch_add(
                    1,
                    std::memory_order_relaxed);
            });
    }

    // Submit LOW tasks afterwards.
    for (int i = 0; i < NUM_LOW; ++i) {
        pool.submit(TaskPriority::LOW,
            [&lowCompleted] {

                std::this_thread::sleep_for(
                    std::chrono::milliseconds(20));

                lowCompleted.fetch_add(
                    1,
                    std::memory_order_relaxed);
            });
    }

    // After a short time, HIGH tasks should dominate.
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    std::cout << "After 150 ms\n";
    std::cout << "HIGH completed: "
              << highCompleted.load() << '\n';
    std::cout << "LOW completed : "
              << lowCompleted.load() << '\n';

    // LOW tasks should eventually execute.
    pool.wait();

    assert(highCompleted == NUM_HIGH);
    assert(lowCompleted == NUM_LOW);

    std::cout << "\n===== Priority Test Passed =====\n";
    std::cout << "High completed: "
              << highCompleted.load() << '\n';

    std::cout << "Low completed : "
              << lowCompleted.load() << '\n';

    std::cout << "Submitted: "
              << pool.submittedTasks() << '\n';

    std::cout << "Completed: "
              << pool.completedTasks() << '\n';

    std::cout << "Queued: "
              << pool.queuedTasks() << '\n';
}