#pragma once

#include <ssh/async/processing_thread.hpp>

#include <set>
#include <future>
#include <functional>
#include <mutex>

namespace SecureShell
{
    /**
     * @brief This class makes it impossible to push tasks to a strand after it has been finalized.
     * Finalization usually means some close operation is happening and no more tasks should be pushed.
     * Like reading a file when the session is closed.
     */
    class ProcessingStrand
    {
      public:
        /**
         * @brief Construct a new Processing Strand object living on top of a processing thread.
         *
         * @param processingThread The processing thread to push tasks to.
         */
        ProcessingStrand(ProcessingThread* processingThread)
            : processingThread_(processingThread)
        {}

        /**
         * @brief Pushes a task, but not if the strand has been finalized and not simultaneously with any other push.
         *
         * @param task The task to push.
         * @return true If the task was pushed.
         * @return false If the strand has been finalized or the task was empty.
         */
        bool pushTask(std::function<void()> task)
        {
            std::scoped_lock lock(mutex_);
            if (finalized_)
                return false;
            return processingThread_->pushTask(std::move(task));
        }

        /**
         * @brief Pushes a task that will be executed until the strand is finalized.
         * But not if the strand has been finalized and not simultaneously with any other push.
         *
         * @param task The task to push.
         * @return std::pair<bool, ProcessingThread::PermanentTaskId> If the task was pushed and the id of the task.
         */
        std::pair<bool, ProcessingThread::PermanentTaskId> pushPermanentTask(std::function<void()> task)
        {
            std::scoped_lock lock(mutex_);
            if (finalized_)
                return {false, ProcessingThread::PermanentTaskId{-1}};
            auto result = processingThread_->pushPermanentTask(std::move(task));
            this->permanentTasks_.insert(result.second);
            return result;
        }

        /**
         * @brief Pushes a task that makes any further pushes impossible. Also removes all permanent tasks.
         *
         * @param task The task to push.
         */
        void pushFinalTask(std::function<void()> task)
        {
            std::scoped_lock lock(mutex_);
            if (finalized_)
                return;
            finalized_ = true;
            for (auto const& id : permanentTasks_)
            {
                processingThread_->removePermanentTask(id);
            }
            processingThread_->pushTask(std::move(task));
        }

        /**
         * @brief Does a final task but runs it on the current calling thread.
         * Otherwise behaves like pushFinalTask.
         *
         * @param task The task to run.
         */
        void doFinalSync(std::function<void()> const& task)
        {
            std::scoped_lock lock(mutex_);
            if (finalized_)
                return;
            finalized_ = true;
            for (auto const& id : permanentTasks_)
            {
                processingThread_->removePermanentTask(id);
            }
            task();
        }

        /**
         * @brief Pushes a task the return of which is returned for a future.
         *
         * @tparam Func The type of the task.
         * @param func The task to push.
         * @return std::future<std::invoke_result_t<std::decay_t<Func>>> The future of the return value of the task.
         */
        template <typename Func>
        auto pushPromiseTask(Func&& func) -> std::future<std::invoke_result_t<std::decay_t<Func>>>
        {
            std::scoped_lock lock(mutex_);
            if (finalized_)
            {
                throw std::runtime_error("Cannot push task to finalized strand.");
            }
            return processingThread_->pushPromiseTask(std::forward<Func>(func));
        }

      private:
        std::recursive_mutex mutex_{};
        bool finalized_ = false;
        ProcessingThread* processingThread_{};
        std::set<ProcessingThread::PermanentTaskId> permanentTasks_{};
    };
}