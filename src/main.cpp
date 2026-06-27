#include "ThreadPool.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

int main() {

    ThreadPool pool(8, 1000);

    std::atomic<int> accepted{0};
    std::atomic<int> rejected{0};

    std::vector<std::thread> producers;

    for (int p = 0; p < 8; ++p) {

        producers.emplace_back([&] {

            while (true) {

                try {

                    pool.submit(
                        TaskPriority::HIGH,
                        [] {
                            std::this_thread::sleep_for(
                                std::chrono::milliseconds(1));
                        });

                    accepted.fetch_add(
                        1,
                        std::memory_order_relaxed);

                } catch (const std::runtime_error&) {

                    rejected.fetch_add(
                        1,
                        std::memory_order_relaxed);

                    break;
                }
            }
        });
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(100));

    std::cout << "Calling shutdown...\n";

    pool.shutdown();

    std::cout << "Shutdown completed.\n";

    for (auto& producer : producers) {
        producer.join();
    }

    std::cout << "\n========== Results ==========\n";

    std::cout << "Accepted: "
              << accepted.load()
              << '\n';

    std::cout << "Rejected: "
              << rejected.load()
              << '\n';

    std::cout << "Submitted Count: "
              << pool.submittedTasks()
              << '\n';

    std::cout << "Completed Count: "
              << pool.completedTasks()
              << '\n';

    std::cout << "Queued Count: "
              << pool.queuedTasks()
              << '\n';
}