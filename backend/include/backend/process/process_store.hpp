#pragma once

#include <backend/process/process.hpp>
#include <backend/process/environment.hpp>
#include <persistence/state/termios.hpp>

#include <nui/rpc.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <chrono>
#include <vector>

class ProcessStore
{
  public:
    ProcessStore(boost::asio::any_io_executor executor);
    ~ProcessStore();

    void registerRpc(Nui::Window& wnd, Nui::RpcHub& hub);

    std::string emplace(
        std::string const& command,
        std::vector<std::string> const& arguments,
        Environment const& environment,
        Persistence::Termios const& termios,
        bool isPty = false,
        std::chrono::seconds defaultExitWaitTimeout = std::chrono::seconds{10});

    std::shared_ptr<Process> operator[](std::string const& id) const
    {
        auto iter = processes_.find(id);
        if (iter == processes_.end())
            return nullptr;
        return iter->second;
    }

    void notifyChildExit(Nui::RpcHub& hub, long long pid);

    void pruneDeadProcesses();

  private:
    boost::asio::any_io_executor executor_;
    std::unordered_map<std::string, std::shared_ptr<Process>> processes_;
    boost::uuids::random_generator uuidGenerator_;
};