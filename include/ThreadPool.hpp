#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <stdexcept>

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;

    bool stopping = false;

    void workerLoop() {
        while(true) {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(queueMutex);

                condition.wait(lock, [this] {
                    return stopping || !tasks.empty();
                });

                if (stopping && tasks.empty()) {
                    return;
                }

                task = std::move(tasks.front());
                tasks.pop();
            }

            task();
        }
    }

public:
    explicit ThreadPool(std::size_t numThreads) {
        if(numThreads == 0) {
            throw std::invalid_argument("ThreadPool must have atleast one thread");
        }

        for (std::size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this]{
                workerLoop();
            });
        }
    }

    ~ThreadPool() {
        shutdown();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void submit(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);

            if (stopping) {
                throw std::runtime_error("Cannot submit task after shutdown");
            }

            tasks.push(std::move(task));
        }

        condition.notify_one();
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(queueMutex);

            if (stopping) {
                return;
            }

            stopping = true;
        }

        condition.notify_all();

        for (std::thread& worker : workers) {
            if(worker.joinable()) {
                worker.join();
            }
        }
    }
};