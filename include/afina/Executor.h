#ifndef AFINA_THREADPOOL_H
#define AFINA_THREADPOOL_H

#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace Afina {

class Executor;
static void perform(Executor *executor);
/**
 * # Thread pool
 */
class Executor {
    enum class State {
        // Threadpool is fully operational, tasks could be added and get executed
        kRun,

        // Threadpool is on the way to be shutdown, no ned task could be added, but existing will be
        // completed as requested
        kStopping,

        // Threadppol is stopped
        kStopped
    };

public:
    Executor(std::string name, size_t low_watermark, size_t high_watermark, size_t max_queue_size,
             std::chrono::milliseconds idle_time)
        : low_watermark(low_watermark), high_watermark(high_watermark), max_queue_size(max_queue_size),
          idle_time(idle_time), idle_threads(0), state(State::kRun) {
        for (auto i = 0; i < low_watermark; i++) {
            threads.emplace_back(perform, this);
        }
    }
    ~Executor() { Stop(true); }

    /**
     * Signal thread pool to stop, it will stop accepting new jobs and close threads just after each become
     * free. All enqueued jobs will be complete.
     *
     * In case if await flag is true, call won't return until all background jobs are done and all threads are stopped
     */
    void Stop(bool await = false) {
        state = State::kStopping;
        empty_condition.notify_all();
        if (await) {
            for (auto &t : this->threads) {
                t.join();
            }
        }
        state = State::kStopped;
    }

    /**
     * Add function to be executed on the threadpool. Method returns true in case if task has been placed
     * onto execution queue, i.e scheduled for execution and false otherwise.
     *
     * That function doesn't wait for function result. Function could always be written in a way to notify caller about
     * execution finished by itself
     */
    template <typename F, typename... Types> bool Execute(F &&func, Types... args) {
        // Prepare "task"
        auto exec = std::bind(std::forward<F>(func), std::forward<Types>(args)...);

        std::unique_lock<std::mutex> lock(this->mutex);
        if (state != State::kRun) {
            return false;
        }

        if (!idle_threads) {
            if (threads.size() < high_watermark) {
                threads.emplace_back(perform, this);
            } else if (tasks.size() == max_queue_size) {
                return false;
            }
        }

        // Enqueue new task
        tasks.push_back(exec);
        empty_condition.notify_one();
        return true;
    }

private:
    // No copy/move/assign allowed
    Executor(const Executor &);            // = delete;
    Executor(Executor &&);                 // = delete;
    Executor &operator=(const Executor &); // = delete;
    Executor &operator=(Executor &&);      // = delete;

    /**
     * Main function that all pool threads are running. It polls internal task queue and execute tasks
     */
    static void perform(Executor *executor) {
        std::function<void()> task;
        while (true) {
            {
                std::unique_lock<std::mutex> lock(executor->mutex);
                executor->idle_threads += 1;
                if (executor->empty_condition.wait_for(lock, executor->idle_time, [&executor](void) {
                        return executor->state == Executor::State::kStopping || executor->tasks.empty();
                    })) {
                    if ((executor->threads.size() > executor->low_watermark || executor->state != State::kRun) &&
                        executor->tasks.empty()) {
                        for (size_t i = 0; i < executor->threads.size(); i++) {
                            if (executor->threads[i].get_id() == std::this_thread::get_id()) {
                                executor->threads.erase(executor->threads.begin() + i);
                                break;
                            }
                        }
                        return;
                    }
                    continue;
                }
                executor->idle_threads -= 1;
                task = executor->tasks.front();
                executor->tasks.pop_front();
            }
            task();
        }
    }

    /**
     * Mutex to protect state below from concurrent modification
     */
    std::mutex mutex;

    /**
     * Conditional variable to await new data in case of empty queue
     */
    std::condition_variable empty_condition;

    /**
     * Vector of actual threads that perorm execution
     */
    std::vector<std::thread> threads;

    /**
     * Task queue
     */
    std::deque<std::function<void()>> tasks;

    /**
     * Flag to stop bg threads
     */
    State state;

    /**
     * Min and max numbers of threads in pool
     */
    size_t low_watermark;
    size_t high_watermark;

    /**
     * Max size of task queue
     */
    size_t max_queue_size;

    /**
     * Time to wait for the new task
     */
    std::chrono::milliseconds idle_time;

    size_t idle_threads;
};

} // namespace Afina

#endif // AFINA_THREADPOOL_H
