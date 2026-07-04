#include "ThreadPool.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

using Clock = std::chrono::steady_clock;

int main() {

    constexpr std::size_t NUM_TASKS = 1'000'000;
    constexpr std::size_t QUEUE_SIZE = NUM_TASKS;

    std::vector<std::size_t> workerCounts = {
        1, 2, 4, 8, 16
    };

    std::cout << "\n========== Scaling Benchmark ==========\n\n";

    std::cout << std::left
              << std::setw(10) << "Workers"
              << std::setw(15) << "Time(s)"
              << std::setw(20) << "Throughput(tasks/s)"
              << '\n';

    std::cout << std::string(45, '-') << '\n';

    for (std::size_t workers : workerCounts) {

        ThreadPool pool(workers, QUEUE_SIZE);

        auto start = Clock::now();

        for (std::size_t i = 0; i < NUM_TASKS; ++i) {

            pool.submitDetached(
                TaskPriority::HIGH,
                [] {

                    volatile int x = 0;

                    for (int i = 0; i < 100; ++i) {
                        x += i;
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