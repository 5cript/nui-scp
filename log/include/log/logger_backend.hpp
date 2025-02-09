#pragma once

#include <log/level.hpp>
#include <log/def.hpp>

#include <nui/backend/rpc_hub.hpp>

#include <mutex>
#include <vector>
#include <string>
#include <utility>

namespace Log
{
    class Logger
    {
      public:
        Logger()
            : guard_{}
            , rpcHub_{nullptr}
            , stash_{}
            , levelStashed_{Level::Info}
        {}

        void setup(Nui::RpcHub* hub)
        {
            std::scoped_lock lock{guard_};
            if (hub == nullptr)
            {
                rpcHub_ = nullptr;
                rpcHubPrelim_ = nullptr;
                return;
            }

            rpcHubPrelim_ = hub;

            rpcHubPrelim_->registerFunction("loggerReady", [this]() {
                std::scoped_lock lock{guard_};
                rpcHub_ = rpcHubPrelim_;
                rpcHubPrelim_ = nullptr;

                if (!stash_.empty())
                {
                    for (auto& s : stash_)
                        logImpl(s.first, s.second);
                    stash_.clear();
                }
                rpcHub_->callRemote("setLogLevel", static_cast<int>(levelStashed_));
            });
            rpcHubPrelim_->registerFunction("log", [](int integralLevel, std::string const& message) {
                spdlog::log(toSpdlogLevel(static_cast<Log::Level>(integralLevel)), message);
            });
            rpcHubPrelim_->registerFunction("setLogLevel", [](int integralLevel) {
                spdlog::set_level(toSpdlogLevel(static_cast<Log::Level>(integralLevel)));
            });
        }

        void setLevel(Log::Level level)
        {
            std::scoped_lock lock{guard_};
            if (rpcHub_ != nullptr)
                rpcHub_->callRemote("setLogLevel", static_cast<int>(level));
            else
            {
                levelStashed_ = level;
            }
            spdlog::set_level(toSpdlogLevel(level));
        }

        Log::Level level() const
        {
            return fromSpdlogLevel(spdlog::get_level());
        }

        template <typename... Args>
        void log(Log::Level level, std::string_view fmt, Args&&... args)
        {
            const std::string buf = spdlog::fmt_lib::format(spdlog::fmt_lib::runtime(fmt), std::forward<Args>(args)...);
            logImpl(level, buf);
        }

        void logImpl(Log::Level level, std::string const& msg)
        {
            decltype(rpcHub_) hub;
            {
                std::scoped_lock lock{guard_};
                hub = rpcHub_;
            }
            if (hub != nullptr)
                hub->callRemote("log", static_cast<int>(level), msg);
            else
            {
                std::scoped_lock lock{guard_};
                stash_.emplace_back(level, msg);
            }

            spdlog::log(toSpdlogLevel(level), msg);
        }

      private:
        std::recursive_mutex guard_;
        Nui::RpcHub* rpcHub_;
        Nui::RpcHub* rpcHubPrelim_;

        // for when the rpc hub is not yet available
        std::vector<std::pair<Log::Level, std::string>> stash_;
        Log::Level levelStashed_;
    };
}