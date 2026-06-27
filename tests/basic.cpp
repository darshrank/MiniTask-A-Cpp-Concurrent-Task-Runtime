#include "ThreadPool.hpp"

#include <cassert>
#include <iostream>
#include <vector>
#include <future>

int main() {

    ThreadPool pool(4, 100);

    std::vector<std::future<int>> futures;

    constexpr int NUM_TASKS = 20;

    for (int i = 0; i < NUM_TASKS; ++i) {
        futures.push_back(
            pool.submit(
                TaskPriority::LOW,
                [i] {
                    return i * i;
                }
            )
        );
    }

    pool.wait();

    int expectedSum = 0;
    int actualSum = 0;

    for (int i = 0; i < NUM_TASKS; ++i) {
        expectedSum += i * i;
        actualSum += futures[i].get();
    }

    assert(actualSum == expectedSum);
    assert(pool.submittedTasks() == NUM_TASKS);
    assert(pool.completedTasks() == NUM_TASKS);
    assert(pool.queuedTasks() == 0);

    std::cout << "==============================\n";
    std::cout << "Basic Execution Test Passed\n";
    std::cout << "==============================\n";
    std::cout << "Submitted : " << pool.submittedTasks() << '\n';
    std::cout << "Completed : " << pool.completedTasks() << '\n';
    std::cout << "Queued    : " << pool.queuedTasks() << '\n';
    std::cout << "Result Sum: " << actualSum << '\n';

    return 0;
}