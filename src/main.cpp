#include "ThreadPool.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    ThreadPool pool(3, 5);

    std::vector<std::future<int>> results;

    // Submit HIGH priority tasks
    for (int i = 0; i < 10; ++i) {
        results.push_back(
            pool.submit(TaskPriority::HIGH, [i] {

                std::this_thread::sleep_for(
                    std::chrono::milliseconds(100));

                std::cout << "[HIGH] Task "
                          << i
                          << " executed\n";

                return i;
            })
        );
    }

    // Submit LOW priority tasks
    for (int i = 100; i < 105; ++i) {
        results.push_back(
            pool.submit(TaskPriority::LOW, [i] {

                std::this_thread::sleep_for(
                    std::chrono::milliseconds(150));

                std::cout << "    [LOW] Task "
                          << i
                          << " executed\n";

                return i;
            })
        );
    }

    // Wait for results
    for (auto& future : results) {
        future.get();
    }

    pool.shutdown();

    std::cout << "\n";
    std::cout << "Submitted: "
              << pool.submittedTasks()
              << "\n";

    std::cout << "Completed: "
              << pool.completedTasks()
              << "\n";

    std::cout << "Queued: "
              << pool.queuedTasks()
              << "\n";

    return 0;
}