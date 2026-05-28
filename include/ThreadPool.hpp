#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <stdexcept>
#include <future>
#include <type_traits>

enum class TaskPriority {
    HIGH,
    LOW
};

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> highPriorityTasks;
    std::queue<std::function<void()>> lowPriorityTasks;

    mutable std::mutex queueMutex;
    std::condition_variable condition;

    std::atomic<std::size_t> submittedCount{0};
    std::atomic<std::size_t> completedCount{0};
    std::size_t highTasksProcessed = 0; //Protected by queue mutex, so no atomic needed.

    bool stopping = false;

    std::size_t maxQueueSize;
    std::condition_variable spaceAvailable;

    void workerLoop() {
        while(true) {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(queueMutex);

                condition.wait(lock, [this] {
                    return stopping || !highPriorityTasks.empty() || !lowPriorityTasks.empty();
                });

                if (stopping && highPriorityTasks.empty() && lowPriorityTasks.empty()) {
                    return;
                }
                
                constexpr std::size_t HIGH_BURST_LIMIT = 3;
                if (!highPriorityTasks.empty() &&
                    (highTasksProcessed < HIGH_BURST_LIMIT ||
                    lowPriorityTasks.empty())) {

                    task = std::move(highPriorityTasks.front());
                    highPriorityTasks.pop();

                    ++highTasksProcessed;

                } else {

                    task = std::move(lowPriorityTasks.front());
                    lowPriorityTasks.pop();

                    highTasksProcessed = 0;
                }
                
                spaceAvailable.notify_one();
            }

            task();
        }
    }

public:
    explicit ThreadPool(std::size_t numThreads, std::size_t queueSize): stopping(false), maxQueueSize(queueSize) {
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

    template <typename Func>
    auto submit(TaskPriority priority, Func&& func) -> std::future<std::invoke_result_t<Func>> {
        using ReturnType = std::invoke_result_t<Func>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::forward<Func>(func)
        );

        std::future<ReturnType> result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queueMutex);

            spaceAvailable.wait(lock, [this, priority] {
                return ( priority == TaskPriority::HIGH 
                    ? highPriorityTasks.size() < maxQueueSize 
                    : lowPriorityTasks.size() < maxQueueSize 
                ) || stopping;
            });

            if (stopping) {
                throw std::runtime_error("Cannot submit task after shutdown");
            }

            if (priority == TaskPriority::HIGH) {
                highPriorityTasks.push([task, this] {
                    (*task)();
                    completedCount.fetch_add(1, std::memory_order_relaxed);
                });
            } else {
                lowPriorityTasks.push([task, this] {
                    (*task)();
                    completedCount.fetch_add(1, std::memory_order_relaxed);
                });
            }

            submittedCount.fetch_add(1, std::memory_order_relaxed);
        }

        condition.notify_one();

        return result;
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

    std::size_t submittedTasks() const {
        return submittedCount.load(std::memory_order_relaxed);
    }

    std::size_t completedTasks() const {
        return completedCount.load(std::memory_order_relaxed);
    }

    std::size_t queuedTasks() const {
        std::lock_guard<std::mutex> lock(queueMutex);
        return highPriorityTasks.size() + lowPriorityTasks.size();
    }
};