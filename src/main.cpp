#include "ThreadPool.hpp"

#include <chrono>
#include <iostream>

int main() {
    ThreadPool pool(4);
    std::mutex printMutex;

    for(int i = 0; i < 10; ++i) {
        pool.submit([i, &printMutex] {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            std::lock_guard<std::mutex> lock(printMutex);

            std::cout << "Task " << i << " executed\n";
        });
    }

    pool.shutdown();
    std::cout << "All tasks completed\n";
    return 0;
}