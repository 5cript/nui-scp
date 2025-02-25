#pragma once

#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <map>

namespace SecureShell
{
    class ProcessingThread
    {
      public:
        constexpr static unsigned int maximumTasksProcessableAtOnce = 10;

        struct PermanentTaskId
        {
            int id = -1;
            friend auto operator<=>(PermanentTaskId const& lhs, PermanentTaskId const& rhs) noexcept
            {
                return lhs.id <=> rhs.id;
            }
        };

        ProcessingThread();
        ~ProcessingThread();
        ProcessingThread(ProcessingThread const&) = delete;
        ProcessingThread& operator=(ProcessingThread const&) = delete;
        ProcessingThread(ProcessingThread&&) = delete;
        ProcessingThread& operator=(ProcessingThread&&) = delete;

        void start(
            std::chrono::milliseconds const& waitCycleTimeout = std::chrono::seconds{1},
            std::chrono::milliseconds const& minimumCycleWait = std::chrono::milliseconds{0});
        void stop();

        bool isRunning() const;

        bool pushTask(std::function<void()> task);

        std::pair<bool, PermanentTaskId> pushPermanentTask(std::function<void()> task);
        bool removePermanentTask(PermanentTaskId const& id);
        int permanentTaskCount() const;
        void clearPermanentTasks();

      private:
        void run(std::chrono::milliseconds const& waitCycleTimeout, std::chrono::milliseconds const& minimumCycleWait);

      private:
        std::thread thread_{};
        mutable std::recursive_mutex taskMutex_{};
        std::atomic<bool> running_ = false;
        std::atomic<bool> shuttingDown_ = false;
        std::atomic_bool permanentTasksAvailable_ = false;
        std::atomic<int> permanentTaskIdCounter_ = 0;
        std::atomic<std::thread::id> processingThreadId_{};
        std::atomic_bool processingPermanents_{false};
        std::vector<std::function<void()>> deferredTaskModification_{};

        std::mutex taskWaitMutex_{};
        std::condition_variable taskCondition_{};
        bool taskAvailable_ = false;

        std::queue<std::function<void()>> tasks_{};
        std::map<PermanentTaskId, std::function<void()>> permanentTasks_{};
    };
}