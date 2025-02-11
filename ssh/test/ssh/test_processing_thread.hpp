#pragma once

#include <ssh/processing_thread.hpp>

#include <gtest/gtest.h>

#include <future>
#include <latch>

using namespace std::chrono_literals;
using namespace std::string_literals;

namespace SecureShell::Test
{
    class ProcessingThreadTest : public ::testing::Test
    {
      protected:
        void SetUp() override
        {}

        void TearDown() override
        {}

      protected:
    };

    TEST_F(ProcessingThreadTest, CanStartThreadAndStopIt)
    {
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
        ASSERT_NO_FATAL_FAILURE(processingThread.stop());
    }

    TEST_F(ProcessingThreadTest, CanStartAndDestructThread)
    {
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
    }

    TEST_F(ProcessingThreadTest, CanPushTask)
    {
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
        processingThread.pushTask([] {});
        ASSERT_NO_FATAL_FAILURE(processingThread.stop());
    }

    TEST_F(ProcessingThreadTest, CanPushMultipleTasks)
    {
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
        for (unsigned int i = 0; i < ProcessingThread::maximumTasksProcessableAtOnce; ++i)
        {
            processingThread.pushTask([] {});
        }
    }

    TEST_F(ProcessingThreadTest, PushedTaskIsEventuallyExecuted)
    {
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
        std::promise<void> promise{};
        EXPECT_TRUE(processingThread.pushTask([&promise] {
            promise.set_value();
        }));
        auto fut = promise.get_future();
        ASSERT_EQ(std::future_status::ready, fut.wait_for(std::chrono::seconds{1}));
    }

    TEST_F(ProcessingThreadTest, AllTasksAreExecutedEvenIfDestructed)
    {
        std::atomic_int counter = 0;
        std::promise<void> promise{};
        {
            ProcessingThread processingThread;
            processingThread.start(std::chrono::milliseconds{1}, std::chrono::milliseconds{100});
            for (unsigned int i = 0; i < ProcessingThread::maximumTasksProcessableAtOnce * 3; ++i)
            {
                EXPECT_TRUE(processingThread.pushTask([&counter, &promise] {
                    if (counter.load() == 0)
                        promise.set_value();
                    ++counter;
                }));
            }
            // wait for at least one task to be executed, otherwise it runs into shutdown immediately and the adds
            // are not processed
            promise.get_future().wait();
        }
        ASSERT_EQ(ProcessingThread::maximumTasksProcessableAtOnce * 3, counter.load());
    }

    TEST_F(ProcessingThreadTest, CanPushTaskAfterStop)
    {
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
        processingThread.stop();
        EXPECT_TRUE(processingThread.pushTask([] {}));
    }

    TEST_F(ProcessingThreadTest, CannotPushTaskWhileStopping)
    {
        ProcessingThread processingThread;
        std::promise<void> promise{};
        processingThread.pushTask([&] {
            promise.set_value();
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
        });
        std::thread asyncStopper{[&] {
            processingThread.stop();
        }};
        promise.get_future().wait();
        EXPECT_FALSE(processingThread.pushTask([] {}));
        asyncStopper.join();
    }

    TEST_F(ProcessingThreadTest, PushedTaskIsExecutedOnStopIfNeverRan)
    {
        ProcessingThread processingThread;
        std::promise<void> promise{};
        processingThread.pushTask([&promise] {
            promise.set_value();
        });
        processingThread.stop();
        auto fut = promise.get_future();
        ASSERT_EQ(std::future_status::ready, fut.wait_for(std::chrono::seconds{1}));
    }

    TEST_F(ProcessingThreadTest, CanPushPermanentTask)
    {
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
        auto result = processingThread.pushPermanentTask([] {});
        EXPECT_TRUE(result.first);
    }

    TEST_F(ProcessingThreadTest, PermanentTaskIsExecuted)
    {
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
        std::promise<void> promise{};
        auto result = processingThread.pushPermanentTask([&promise] {
            promise.set_value();
        });
        auto fut = promise.get_future();
        ASSERT_EQ(std::future_status::ready, fut.wait_for(std::chrono::seconds{1}));
    }

    TEST_F(ProcessingThreadTest, PermanentTaskIsExecutedMultipleTimes)
    {
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
        std::latch latch{5};
        std::atomic_int counter = 5;
        auto result = processingThread.pushPermanentTask([&] {
            if (counter.load() == 0)
                return;
            latch.count_down();
            --counter;
        });
        bool waitResult = false;
        for (int tryWaitCount = 0; tryWaitCount < 5; ++tryWaitCount)
        {
            if (waitResult = latch.try_wait(); waitResult)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
        }
        EXPECT_TRUE(waitResult);
    }

    TEST_F(ProcessingThreadTest, TaskIsExecutedIfIfPermantentTaskIsRunning)
    {
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
        std::promise<void> promise{};
        auto result = processingThread.pushPermanentTask([] {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
        });
        processingThread.pushTask([&promise] {
            promise.set_value();
        });
        auto fut = promise.get_future();
        ASSERT_EQ(std::future_status::ready, fut.wait_for(std::chrono::seconds{1}));
    }

    TEST_F(ProcessingThreadTest, TaskIsOnlyExecutedOnceIfPermanentTaskIsRunning)
    {
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
        std::atomic_int counter = 0;
        std::promise<void> promise{};
        auto result = processingThread.pushPermanentTask([&counter, &promise] {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
            ++counter;
            if (counter.load() == 5)
                promise.set_value();
        });

        std::atomic_int taskCounter = 0;
        processingThread.pushTask([&taskCounter] {
            ++taskCounter;
        });

        auto fut = promise.get_future();
        ASSERT_EQ(std::future_status::ready, fut.wait_for(std::chrono::seconds{1}));
        EXPECT_EQ(1, taskCounter.load());
    }

    TEST_F(ProcessingThreadTest, CanRemovePermanentTaskWhenNotRunning)
    {
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
        auto result = processingThread.pushPermanentTask([] {});
        processingThread.removePermanentTask(result.second);
    }

    TEST_F(ProcessingThreadTest, CanRemovePermanentTaskWhenRunning)
    {
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
        std::promise<void> promise{};
        auto result = processingThread.pushPermanentTask([&promise] {
            promise.set_value();
        });
        auto fut = promise.get_future();
        ASSERT_EQ(std::future_status::ready, fut.wait_for(std::chrono::seconds{1}));
        ASSERT_NO_FATAL_FAILURE(processingThread.removePermanentTask(result.second));
    }

    TEST_F(ProcessingThreadTest, CanRemovePermanentTaskFromRegularTask)
    {
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
        std::promise<void> promise{};
        auto result = processingThread.pushPermanentTask([&promise] {
            promise.set_value();
        });
        processingThread.pushTask([&processingThread, result] {
            processingThread.removePermanentTask(result.second);
        });
        auto fut = promise.get_future();
        ASSERT_EQ(std::future_status::ready, fut.wait_for(std::chrono::seconds{1}));
    }

    TEST_F(ProcessingThreadTest, CanAddPermanentTaskFromRegularTask)
    {
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
        std::promise<void> promise{};
        processingThread.pushTask([&processingThread, &promise] {
            auto result = processingThread.pushPermanentTask([&promise] {
                promise.set_value();
            });
        });
        auto fut = promise.get_future();
        ASSERT_EQ(std::future_status::ready, fut.wait_for(std::chrono::seconds{1}));
    }

    TEST_F(ProcessingThreadTest, CanAddRegularTaskFromRegularTask)
    {
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
        std::promise<void> promise{};
        processingThread.pushTask([&processingThread, &promise] {
            auto result = processingThread.pushTask([&promise] {
                promise.set_value();
            });
        });
        auto fut = promise.get_future();
        ASSERT_EQ(std::future_status::ready, fut.wait_for(std::chrono::seconds{1}));
    }

    TEST_F(ProcessingThreadTest, CanRemovePermanentTaskFromPermanentTask)
    {
        std::promise<void> promise{};
        std::atomic_bool removeExecuted = false;
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
        std::shared_ptr<std::pair<bool, ProcessingThread::PermanentTaskId>> result =
            std::make_shared<std::pair<bool, ProcessingThread::PermanentTaskId>>();
        *result = processingThread.pushPermanentTask([&processingThread, result, &removeExecuted] {
            removeExecuted = true;
            processingThread.removePermanentTask(result->second);
            /*
             * DONT DO ANYTHING HERE WITH CAPTURES, AS THEY ARE DESTROYED
             */
        });
        processingThread.pushTask([&promise]() {
            promise.set_value();
        });
        auto fut = promise.get_future();
        ASSERT_EQ(std::future_status::ready, fut.wait_for(std::chrono::seconds{1}));
        processingThread.stop();
        EXPECT_EQ(0, processingThread.permanentTaskCount());
        EXPECT_TRUE(removeExecuted);
    }

    TEST_F(ProcessingThreadTest, RemovedPermanentTaskIsNoLongerCalled)
    {
        std::promise<void> promise{};
        std::atomic_bool promiseSet = false;
        std::atomic_int counter = 0;
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
        std::shared_ptr<std::pair<bool, ProcessingThread::PermanentTaskId>> result =
            std::make_shared<std::pair<bool, ProcessingThread::PermanentTaskId>>();
        *result = processingThread.pushPermanentTask([&]() {
            if (!promiseSet)
                promise.set_value();
            promiseSet = true;
            ++counter;
            processingThread.removePermanentTask(result->second);
        });

        auto fut = promise.get_future();
        ASSERT_EQ(std::future_status::ready, fut.wait_for(std::chrono::seconds{1}));
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        processingThread.stop();
        EXPECT_EQ(1, counter.load());
    }

    TEST_F(ProcessingThreadTest, CanSafelyDoubleDeletePermanentTask)
    {
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
        auto result = processingThread.pushPermanentTask([] {});
        EXPECT_TRUE(processingThread.removePermanentTask(result.second));
        EXPECT_FALSE(processingThread.removePermanentTask(result.second));
    }

    TEST_F(ProcessingThreadTest, CanClearPermanentTasks)
    {
        ProcessingThread processingThread;
        processingThread.start(std::chrono::milliseconds{1});
        processingThread.pushPermanentTask([] {});
        processingThread.clearPermanentTasks();
        EXPECT_EQ(0, processingThread.permanentTaskCount());
    }
}