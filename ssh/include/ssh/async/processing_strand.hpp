#pragma once

#include <ssh/async/processing_thread.hpp>

#include <set>
#include <future>
#include <functional>
#include <mutex>

namespace SecureShell
{
    class ProcessingStrand
    {
      public:
        ProcessingStrand(ProcessingThread* processingThread)
            : processingThread_(processingThread)
        {}

        bool pushTask(std::function<void()> task)
        {
            std::scoped_lock lock(mutex_);
            if (finalized_)
            {
                return false;
            }
            return processingThread_->pushTask(std::move(task));
        }

        std::pair<bool, ProcessingThread::PermanentTaskId> pushPermanentTask(std::function<void()> task)
        {
            std::scoped_lock lock(mutex_);
            if (finalized_)
            {
                return {false, ProcessingThread::PermanentTaskId{-1}};
            }
            auto result = processingThread_->pushPermanentTask(std::move(task));
            this->permanentTasks_.insert(result.second);
            return result;
        }

        void pushFinalTask(std::function<void()> task)
        {
            std::scoped_lock lock(mutex_);
            if (finalized_)
            {
                return;
            }
            finalized_ = true;
            for (auto const& id : permanentTasks_)
            {
                processingThread_->removePermanentTask(id);
            }
            processingThread_->pushTask(std::move(task));
        }

        void doFinalSync(std::function<void()> const& task)
        {
            std::scoped_lock lock(mutex_);
            if (finalized_)
            {
                return;
            }
            finalized_ = true;
            for (auto const& id : permanentTasks_)
            {
                processingThread_->removePermanentTask(id);
            }
            task();
        }

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