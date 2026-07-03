#include "ThreadPool.hpp"

#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <unordered_set>
#include <vector>

int main() {
    constexpr std::size_t NUM_WORKERS = 4;
    constexpr std::size_t QUEUE_SIZE = 100;
    constexpr std::size_t NUM_TASKS = 20;

    ThreadPool pool(NUM_WORKERS, QUEUE_SIZE);

    // Advance nextQueue so the next submission starts at queue 0.
    for (std::size_t i = 0; i < NUM_WORKERS; ++i) {
        pool.submit(TaskPriority::LOW, [] {}).get();
    }

    pool.wait();

    std::mutex threadSetMutex;
    std::unordered_set<std::thread::id> workerThreads;

    std::vector<std::future<void>> futures;

    // Every submission now goes to queue 0.
    // Wait between submissions so worker 0 cannot drain everything
    // before the others begin stealing.
    for (std::size_t i = 0; i < NUM_TASKS; ++i) {

        futures.push_back(
            pool.submit(TaskPriority::HIGH,
                [&]() {

                    {
                        std::lock_guard<std::mutex> lock(threadSetMutex);
                        workerThreads.insert(std::this_thread::get_id());
                    }

                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(100));
                }));

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    for (auto& future : futures) {
        future.get();
    }

    pool.wait();

    std::cout << "\n===== Work Stealing Test =====\n";

    std::cout << "Unique worker threads used: "
              << workerThreads.size()
              << '\n';

    std::cout << "Submitted: "
              << pool.submittedTasks()
              << '\n';

    std::cout << "Completed: "
              << pool.completedTasks()
              << '\n';

    std::cout << "Queued: "
              << pool.queuedTasks()
              << '\n';

    if (workerThreads.size() > 1) {
        std::cout << "\n===== Work Stealing Test Passed =====\n";
        return 0;
    }

    std::cerr << "\nWork stealing did not occur!\n";
    return 1;
}

//One caveat: this is only a probabilistic test. It infers that work stealing happened because multiple worker threads executed tasks, 
//but it does not prove the tasks were stolen from another worker's queue.