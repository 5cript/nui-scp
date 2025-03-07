#include <ssh/async/processing_thread.hpp>

#include <stdexcept>
#include <future>

namespace SecureShell
{
    ProcessingThread::ProcessingThread()
    {}
    ProcessingThread::~ProcessingThread()
    {
        stop();
    }
    bool ProcessingThread::isRunning() const
    {
        return running_;
    }
    void ProcessingThread::start(
        std::chrono::milliseconds const& waitCycleTimeout,
        std::chrono::milliseconds const& minimumCycleWait)
    {
        running_ = true;
        shuttingDown_ = false;
        std::promise<void> awaitThreadStart{};
        thread_ = std::thread([this, &awaitThreadStart, waitCycleTimeout, minimumCycleWait] {
            awaitThreadStart.set_value();
            processingThreadId_.store(std::this_thread::get_id());
            run(waitCycleTimeout, minimumCycleWait);
        });
        awaitThreadStart.get_future().wait();
    }
    void ProcessingThread::stop()
    {
        shuttingDown_ = true;
        running_ = false;
        if (thread_.joinable())
            thread_.join();

        // execute all pending tasks:
        std::lock_guard lock{taskMutex_};
        while (!tasks_.empty())
        {
            tasks_.front()();
            tasks_.pop();
        }
        shuttingDown_ = false;
    }
    bool ProcessingThread::pushTask(std::function<void()> task)
    {
        if (!task)
        {
            throw std::invalid_argument("Task must not be empty.");
        }

        if (shuttingDown_)
        {
            return false;
        }

        {
            std::lock_guard lock{taskMutex_};
            tasks_.push(std::move(task));
        }
        {
            std::lock_guard lock{taskWaitMutex_};
            taskAvailable_ = true;
        }
        taskCondition_.notify_one();
        return true;
    }
    std::pair<bool, ProcessingThread::PermanentTaskId> ProcessingThread::pushPermanentTask(std::function<void()> task)
    {
        if (!task)
        {
            throw std::invalid_argument("Task must not be empty.");
        }

        if (shuttingDown_)
        {
            return {false, PermanentTaskId{-1}};
        }

        PermanentTaskId id{-1};
        {
            std::lock_guard lock{taskMutex_};
            ++permanentTaskIdCounter_;
            id = PermanentTaskId{permanentTaskIdCounter_};
            permanentTasks_.insert({id, std::move(task)});
            permanentTasksAvailable_ = true;
        }
        return {true, id};
    }
    void ProcessingThread::clearPermanentTasks()
    {
        std::lock_guard lock{taskMutex_};
        if (processingPermanents_)
        {
            deferredTaskModification_.push_back([this]() {
                permanentTasks_.clear();
                permanentTasksAvailable_ = false;
            });
            return;
        }

        permanentTasks_.clear();
        permanentTasksAvailable_ = false;
    }
    bool ProcessingThread::removePermanentTask(PermanentTaskId const& id)
    {
        std::lock_guard lock{taskMutex_};
        if (processingPermanents_)
        {
            deferredTaskModification_.push_back([this, id]() {
                permanentTasks_.erase(id);
                permanentTasksAvailable_ = !permanentTasks_.empty();
            });
            return true;
        }

        auto result = permanentTasks_.erase(id);
        permanentTasksAvailable_ = !permanentTasks_.empty();
        return result > 0;
    }
    void ProcessingThread::run(
        std::chrono::milliseconds const& waitCycleTimeout,
        std::chrono::milliseconds const& minimumCycleWait)
    {
        auto timePoint = std::chrono::steady_clock::now();

        try
        {
            while (running_)
            {
                timePoint = std::chrono::steady_clock::now();

                if (permanentTasksAvailable_)
                {
                    std::unique_lock lock(taskMutex_);
                    const auto permaTasksMoved = std::move(permanentTasks_);
                    permanentTasks_ = {};
                    processingPermanents_ = true;
                    lock.unlock();

                    for (auto const& [_, task] : permaTasksMoved)
                    {
                        // Task is checked before adding, shouldnt possibly be empty:
                        task();

                        // Stop running if shutdown was requested:
                        if (!running_ || shuttingDown_)
                            break;
                    }

                    lock.lock();
                    processingPermanents_ = false;
                    if (permanentTasks_.empty())
                        permanentTasks_ = std::move(permaTasksMoved);
                    else
                    {
                        // Move em back the expensive way:
                        for (auto& [id, task] : permaTasksMoved)
                        {
                            permanentTasks_.insert({id, std::move(task)});
                        }
                    }
                    permanentTasksAvailable_ = !permanentTasks_.empty();
                    for (auto& modify : deferredTaskModification_)
                    {
                        modify();
                    }
                }
                else
                {
                    std::unique_lock lock(taskWaitMutex_);
                    if (!taskAvailable_)
                    {
                        bool result = taskCondition_.wait_for(lock, waitCycleTimeout, [this] {
                            return taskAvailable_;
                        });
                        if (!result)
                            continue;
                    }
                }

                std::vector<std::function<void()>> tasks{};
                {
                    std::lock_guard lock(taskMutex_);
                    for (int i = 0; i != maximumTasksProcessableAtOnce && !tasks_.empty(); ++i)
                    {
                        tasks.push_back(std::move(tasks_.front()));
                        tasks_.pop();
                    }
                }

                for (auto& task : tasks)
                {
                    // Task is checked before adding, shouldnt possibly be empty:
                    task();
                }

                {
                    std::lock_guard lock{taskWaitMutex_};
                    taskAvailable_ = !tasks_.empty();
                }

                if (minimumCycleWait.count() > 0)
                {
                    auto now = std::chrono::steady_clock::now();
                    auto diff = now - timePoint;
                    if (diff < minimumCycleWait)
                    {
                        std::this_thread::sleep_for(minimumCycleWait - diff);
                    }
                }
            }
        }
        catch (...)
        {
            running_ = false;
        }
    }
    int ProcessingThread::permanentTaskCount() const
    {
        std::lock_guard lock{taskMutex_};
        return permanentTasks_.size();
    }
    bool ProcessingThread::awaitCycle(std::chrono::milliseconds maxWait)
    {
        if (!withinProcessingThread() && running_)
        {
            return pushPromiseTask([]() {
                       return true;
                   }).wait_for(maxWait) == std::future_status::ready;
        }
        return false;
    }
}