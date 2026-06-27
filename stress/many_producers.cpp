#include "ThreadPool.hpp"

#include <chrono>
#include <future>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

int main() {

    constexpr std::size_t NUM_WORKERS = 8;
    constexpr std::size_t NUM_PRODUCERS = 8;
    constexpr std::size_t TASKS_PER_PRODUCER = 10000;

    ThreadPool pool(NUM_WORKERS, 1000);

    std::vector<std::thread> producers;

    std::vector<std::future<int>> futures;
    std::mutex futuresMutex;

    auto start = std::chrono::steady_clock::now();

    for (std::size_t producerId = 0;
         producerId < NUM_PRODUCERS;
         ++producerId) {

        producers.emplace_back([&, producerId] {

            std::mt19937 rng(
                static_cast<unsigned>(
                    std::chrono::steady_clock::now()
                        .time_since_epoch()
                        .count() +
                    producerId));

            std::uniform_int_distribution<int> dist(0, 100);

            std::vector<std::future<int>> localFutures;
            localFutures.reserve(TASKS_PER_PRODUCER);

            for (std::size_t i = 0;
                 i < TASKS_PER_PRODUCER;
                 ++i) {

                std::size_t taskId =
                    producerId * TASKS_PER_PRODUCER + i;

                localFutures.push_back(
                    pool.submit(
                        taskId % 2 == 0
                            ? TaskPriority::HIGH
                            : TaskPriority::LOW,
                        [taskId, seed = static_cast<unsigned>(taskId + producerId)]() mutable {

                            std::mt19937 rng(seed);
                            std::uniform_int_distribution<int> dist(0, 100);

                            std::this_thread::sleep_for(
                                std::chrono::microseconds(
                                    dist(rng)));

                            return static_cast<int>(taskId);
                        })
                );
            }

            {
                std::lock_guard<std::mutex> lock(futuresMutex);

                for (auto& future : localFutures) {
                    futures.push_back(std::move(future));
                }
            }
        });
    }

    for (auto& producer : producers) {
        producer.join();
    }

    long long sum = 0;

    for (auto& future : futures) {
        sum += future.get();
    }

    auto end = std::chrono::steady_clock::now();

    std::cout << "\n========== Random Sleep Stress Test ==========\n";

    std::cout << "Submitted: "
              << pool.submittedTasks()
              << '\n';

    std::cout << "Completed: "
              << pool.completedTasks()
              << '\n';

    std::cout << "Queued: "
              << pool.queuedTasks()
              << '\n';

    std::cout << "Total Futures: "
              << futures.size()
              << '\n';

    std::cout << "Sum: "
              << sum
              << '\n';

    std::cout << "Elapsed Time(ms): "
              << std::chrono::duration_cast<
                     std::chrono::milliseconds>(
                     end - start)
                     .count()
              << '\n';
}