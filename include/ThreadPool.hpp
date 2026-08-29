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
#include <iostream>
#include <random>

enum class TaskPriority {
    HIGH,
    LOW
};

enum class StealPolicy {
    RANDOM,
    LINEAR_SCAN
};

struct WorkerQueue {
    std::queue<std::function<void()>> highPriorityTasks;
    std::queue<std::function<void()>> lowPriorityTasks;
    std::mutex mutex;
};

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::vector<std::unique_ptr<WorkerQueue>> workerQueues;
    std::vector<std::size_t> highTasksProcessed;

    mutable std::mutex stateMutex;
    std::condition_variable condition;

    std::atomic<std::size_t> submittedCount{0};
    std::atomic<std::size_t> completedCount{0};
    std::atomic<std::size_t> nextQueue{0};
    std::atomic<std::size_t> totalQueuedTasks{0};

    std::atomic<bool> stopping{false};

    std::size_t maxQueueSize;
    std::condition_variable spaceAvailable;

    std::condition_variable completionCondition;

    std::atomic<std::size_t> stealAttempts{0};
    std::atomic<std::size_t> successfulSteals{0};

    StealPolicy stealPolicy;
    
    void workerLoop(std::size_t workerId) {
        constexpr std::size_t HIGH_BURST_LIMIT = 3;

        while (true) {
            std::function<void()> task;

            // Sleep until work exists or shutdown begins
            {
                std::unique_lock<std::mutex> stateLock(stateMutex);

                condition.wait(stateLock, [this] {
                    return stopping.load(std::memory_order_relaxed) ||
                        totalQueuedTasks.load(std::memory_order_relaxed) > 0;
                });

                if (stopping.load(std::memory_order_relaxed) &&
                    totalQueuedTasks.load(std::memory_order_relaxed) == 0) {
                    return;
                }
            }

            // Try own queue first
            {
                std::lock_guard<std::mutex> queueLock(
                    workerQueues[workerId]->mutex);

                if (!workerQueues[workerId]->highPriorityTasks.empty() &&
                    (highTasksProcessed[workerId] < HIGH_BURST_LIMIT ||
                    workerQueues[workerId]->lowPriorityTasks.empty())) {

                    task = std::move(
                        workerQueues[workerId]->highPriorityTasks.front());

                    workerQueues[workerId]->highPriorityTasks.pop();

                    highTasksProcessed[workerId]++;

                    totalQueuedTasks.fetch_sub(
                        1,
                        std::memory_order_relaxed);
                }
                else if (!workerQueues[workerId]->lowPriorityTasks.empty()) {

                    task = std::move(
                        workerQueues[workerId]->lowPriorityTasks.front());

                    workerQueues[workerId]->lowPriorityTasks.pop();

                    highTasksProcessed[workerId] = 0;

                    totalQueuedTasks.fetch_sub(
                        1,
                        std::memory_order_relaxed);
                }
            }

            // Work stealing
            if (!task && stealPolicy == StealPolicy::LINEAR_SCAN && workerQueues.size() > 1) {

                for (std::size_t i = 0;
                    i < workerQueues.size();
                    ++i) {

                    if (i == workerId) {
                        continue;
                    }

                    stealAttempts.fetch_add(1, std::memory_order_relaxed);

                    std::lock_guard<std::mutex> victimLock(
                        workerQueues[i]->mutex);

                    if (!workerQueues[i]->highPriorityTasks.empty() &&
                        (highTasksProcessed[workerId] < HIGH_BURST_LIMIT ||
                        workerQueues[i]->lowPriorityTasks.empty())) {

                        task = std::move(
                            workerQueues[i]->highPriorityTasks.front());

                        workerQueues[i]->highPriorityTasks.pop();

                        highTasksProcessed[workerId]++;

                        totalQueuedTasks.fetch_sub(
                            1,
                            std::memory_order_relaxed);
                        
                        successfulSteals.fetch_add(1, std::memory_order_relaxed);
                        break;
                    }

                    if (!workerQueues[i]->lowPriorityTasks.empty()) {

                        task = std::move(
                            workerQueues[i]->lowPriorityTasks.front());

                        workerQueues[i]->lowPriorityTasks.pop();

                        highTasksProcessed[workerId] = 0;

                        totalQueuedTasks.fetch_sub(
                            1,
                            std::memory_order_relaxed);

                        break;
                    }
                }
            }

            // Work stealing
            if (!task && stealPolicy == StealPolicy::RANDOM && workerQueues.size() > 1) {

                stealAttempts.fetch_add(1, std::memory_order_relaxed);
                const std::size_t numWorkers = workerQueues.size();

                thread_local std::mt19937 rng(std::random_device{}());

                std::uniform_int_distribution<std::size_t> dist(0, numWorkers - 2);

                std::size_t victim = dist(rng);

                // Skip ourselves
                if (victim >= workerId) {
                    ++victim;
                }

                std::lock_guard<std::mutex> victimLock(
                    workerQueues[victim]->mutex);

                if (!workerQueues[victim]->highPriorityTasks.empty() &&
                    (highTasksProcessed[workerId] < HIGH_BURST_LIMIT ||
                    workerQueues[victim]->lowPriorityTasks.empty())) {

                    task = std::move(
                        workerQueues[victim]->highPriorityTasks.front());

                    workerQueues[victim]->highPriorityTasks.pop();

                    highTasksProcessed[workerId]++;

                    totalQueuedTasks.fetch_sub(
                        1,
                        std::memory_order_relaxed);
                    
                    successfulSteals.fetch_add(
                        1,
                        std::memory_order_relaxed);
                }
                else if (!workerQueues[victim]->lowPriorityTasks.empty()) {

                    task = std::move(
                        workerQueues[victim]->lowPriorityTasks.front());

                    workerQueues[victim]->lowPriorityTasks.pop();

                    highTasksProcessed[workerId] = 0;

                    totalQueuedTasks.fetch_sub(
                        1,
                        std::memory_order_relaxed);
                    
                    successfulSteals.fetch_add(
                        1,
                        std::memory_order_relaxed);
                }
            }

            if (task) {
                spaceAvailable.notify_one();

                try {
                    task();
                }
                catch (const std::exception& ex) {
                    std::cerr << "Worker caught exception: "
                            << ex.what() << '\n';
                }
                catch (...) {
                    std::cerr << "Worker caught unknown exception\n";
                }
            }
        }
    }

public:
    explicit ThreadPool(std::size_t numThreads, std::size_t queueSize, StealPolicy policy = StealPolicy::RANDOM): 
        stopping(false), maxQueueSize(queueSize), stealPolicy(policy) {
        if(numThreads == 0) {
            throw std::invalid_argument("ThreadPool must have atleast one thread");
        }

        highTasksProcessed.resize(numThreads, 0);

        for (std::size_t i = 0; i < numThreads; ++i) {
            workerQueues.push_back(
                std::make_unique<WorkerQueue>()
            );
        }

        for (std::size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this, i]{
                workerLoop(i);
            });
        }
    }

    ~ThreadPool() {
        shutdown();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void wait() {
        const std::size_t target = submittedCount.load(std::memory_order_relaxed);

        std::unique_lock<std::mutex> lock(stateMutex);

        completionCondition.wait(lock, [this, target] {
            return completedCount.load(std::memory_order_relaxed) >= target;
        });
    }

    template <typename Func>
    auto submit(TaskPriority priority, Func&& func) -> std::future<std::invoke_result_t<Func>> {
        using ReturnType = std::invoke_result_t<Func>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::forward<Func>(func)
        );

        std::future<ReturnType> result = task->get_future();
        std::size_t queueIndex = nextQueue.fetch_add(1) % workerQueues.size();

        {
            std::unique_lock<std::mutex> lock(workerQueues[queueIndex]->mutex);    

            spaceAvailable.wait(lock, [this, priority, queueIndex] {
                return ( workerQueues[queueIndex]->highPriorityTasks.size() + 
                workerQueues[queueIndex]->lowPriorityTasks.size()  
                ) < maxQueueSize || stopping.load(std::memory_order_relaxed);
            });

            if (stopping.load(std::memory_order_relaxed)) {
                throw std::runtime_error("Cannot submit task after shutdown");
            }

            if (priority == TaskPriority::HIGH) {
                workerQueues[queueIndex]->highPriorityTasks.push([task, this] {
                    (*task)();
                    completedCount.fetch_add(1, std::memory_order_relaxed);

                    completionCondition.notify_all();
                });
            } else {
                workerQueues[queueIndex]->lowPriorityTasks.push([task, this] {
                    (*task)();
                    completedCount.fetch_add(1, std::memory_order_relaxed);

                    completionCondition.notify_all();
                });
            }
            
            totalQueuedTasks.fetch_add(1, std::memory_order_relaxed);
            submittedCount.fetch_add(1, std::memory_order_relaxed);
        }

        condition.notify_one();

        return result;
    }

    void shutdown() {
        if (stopping.exchange(true, std::memory_order_relaxed)) {
            return;
        }

        condition.notify_all();
        spaceAvailable.notify_all();
        completionCondition.notify_all();

        for (std::thread& worker : workers) {
            if (worker.joinable()) {
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
        return totalQueuedTasks.load(std::memory_order_relaxed);
    }

    template <typename Func>
    void submitDetached(TaskPriority priority, Func&& func) {
        std::size_t queueIndex =
            nextQueue.fetch_add(1, std::memory_order_relaxed)
            % workerQueues.size();

        {
            std::unique_lock<std::mutex> lock(workerQueues[queueIndex]->mutex);

            spaceAvailable.wait(lock, [this, queueIndex] {
                return (workerQueues[queueIndex]->highPriorityTasks.size() +
                        workerQueues[queueIndex]->lowPriorityTasks.size())
                        < maxQueueSize
                    || stopping.load(std::memory_order_relaxed);
            });

            if (stopping.load(std::memory_order_relaxed)) {
                throw std::runtime_error("Cannot submit task after shutdown");
            }

            auto wrapper = [func = std::forward<Func>(func), this]() mutable {

                try {
                    func();
                }
                catch (...) {
                }

                completedCount.fetch_add(1, std::memory_order_relaxed);

                completionCondition.notify_all();
            };

            if (priority == TaskPriority::HIGH) {
                workerQueues[queueIndex]->highPriorityTasks.push(std::move(wrapper));
            }
            else {
                workerQueues[queueIndex]->lowPriorityTasks.push(std::move(wrapper));
            }

            totalQueuedTasks.fetch_add(1, std::memory_order_relaxed);
            submittedCount.fetch_add(1, std::memory_order_relaxed);
        }

        condition.notify_one();
    }

    std::size_t getStealAttempts() const {
        return stealAttempts.load(std::memory_order_relaxed);
    }

    std::size_t getSuccessfulSteals() const {
        return successfulSteals.load(std::memory_order_relaxed);
    }
    };