#include "ThreadPool.hpp"

#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    constexpr std::size_t NUM_WORKERS = 1;
    constexpr std::size_t QUEUE_SIZE = 2;

    ThreadPool pool(NUM_WORKERS, QUEUE_SIZE);

    std::vector<std::future<void>> futures;

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < 5; ++i) {
        futures.push_back(
            pool.submit(TaskPriority::HIGH, [i] {

                std::cout << "Executing task "
                          << i
                          << '\n';

                std::this_thread::sleep_for(
                    std::chrono::milliseconds(200));
            })
        );

        std::cout << "Submitted task "
                  << i
                  << '\n';
    }

    auto endSubmit = std::chrono::steady_clock::now();

    for (auto& future : futures) {
        future.get();
    }

    pool.wait();

    auto end = std::chrono::steady_clock::now();

    auto submitTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            endSubmit - start)
            .count();

    auto totalTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start)
            .count();

    std::cout << "\n===== Queue Saturation Test =====\n";

    std::cout << "Submit time : "
              << submitTime
              << " ms\n";

    std::cout << "Total time  : "
              << totalTime
              << " ms\n";

    std::cout << "Submitted   : "
              << pool.submittedTasks()
              << '\n';

    std::cout << "Completed   : "
              << pool.completedTasks()
              << '\n';

    std::cout << "Queued      : "
              << pool.queuedTasks()
              << '\n';

    if (submitTime >= 300 &&
        pool.submittedTasks() == 5 &&
        pool.completedTasks() == 5 &&
        pool.queuedTasks() == 0)
    {
        std::cout << "\n===== Queue Saturation Test Passed =====\n";
        return 0;
    }

    std::cerr << "\nQueue saturation test failed!\n";
    return 1;
}