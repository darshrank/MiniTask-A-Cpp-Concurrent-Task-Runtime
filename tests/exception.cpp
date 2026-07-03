#include "ThreadPool.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>

int main() {
    ThreadPool pool(4, 100);

    auto good1 = pool.submit(TaskPriority::LOW, [] {
        return 10;
    });

    auto bad = pool.submit(TaskPriority::HIGH, []() -> int {
        throw std::runtime_error("Intentional failure");
    });

    auto good2 = pool.submit(TaskPriority::LOW, [] {
        return 20;
    });

    assert(good1.get() == 10);

    bool exceptionCaught = false;

    try {
        bad.get();
    }
    catch (const std::runtime_error& e) {
        exceptionCaught = true;
        std::cout << "Caught expected exception: "
                  << e.what() << '\n';
    }

    assert(exceptionCaught);

    assert(good2.get() == 20);

    pool.wait();

    assert(pool.submittedTasks() == 3);
    assert(pool.completedTasks() == 3);
    assert(pool.queuedTasks() == 0);

    std::cout << "\n===== Exception Test Passed =====\n";
}