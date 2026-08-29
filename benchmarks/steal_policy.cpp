#include "ThreadPool.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>
#include <string>

using Clock = std::chrono::steady_clock;

int main(int argc, char* argv[]) {
    StealPolicy policy = StealPolicy::RANDOM;

    if (argc > 1 && std::string(argv[1]) == "linear") {
        policy = StealPolicy::LINEAR_SCAN;
    }

    std::size_t taskIterations = 100;

    if (argc > 2) {
        taskIterations = std::stoull(argv[2]);
    }
    constexpr std::size_t NUM_TASKS = 1'000'000;
    constexpr std::size_t QUEUE_SIZE = NUM_TASKS;

    std::vector<std::size_t> workerCounts = {
        1, 2, 4, 8, 16
    };

    std::cout << "\n========== Scaling Benchmark ==========\n\n";
    std::cout << "Policy: "
          << (policy == StealPolicy::RANDOM ? "random" : "linear")
          << "\n\n";
    std::cout << "Task iterations: "
          << taskIterations
          << "\n\n";
    std::cout << std::left
              << std::setw(10) << "Workers"
              << std::setw(15) << "Time(s)"
              << std::setw(20) << "Throughput(tasks/s)"
              << '\n';

    std::cout << std::string(45, '-') << '\n';

    for (std::size_t workers : workerCounts) {

        ThreadPool pool(workers, QUEUE_SIZE, policy);

        auto start = Clock::now();

        for (std::size_t i = 0; i < NUM_TASKS; ++i) {

            pool.submitDetached(
                TaskPriority::HIGH,
                [taskIterations] {

                    volatile int x = 0;

                    for (std::size_t i = 0; i < taskIterations; ++i) {
                        x += static_cast<int>(i);
                    }
                });
        }

        pool.wait();

        auto end = Clock::now();

        double elapsed =
            std::chrono::duration<double>(end - start).count();

        double throughput =
            static_cast<double>(NUM_TASKS) / elapsed;

        std::cout << std::left
                  << std::setw(10) << workers
                  << std::setw(15) << std::fixed
                  << std::setprecision(3) << elapsed
                  << std::setw(20) << std::fixed
                  << std::setprecision(0) << throughput
                  << '\n';

        std::cout << "Steal Attempts    : "
          << pool.getStealAttempts() << '\n';

        std::cout << "Successful Steals : "
                << pool.getSuccessfulSteals() << '\n';

    }

    
    return 0;
}