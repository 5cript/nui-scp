#pragma once

#include <backend/sftp/operation.hpp>
#include <backend/sftp/download_operation.hpp>
#include <persistence/state/state.hpp>
#include <ssh/sftp_session.hpp>
#include <ids/ids.hpp>

#include <vector>
#include <filesystem>
#include <memory>
#include <utility>

class OperationQueue
{
  public:
    OperationQueue();

    using Error = OperationErrorType;

    void start();
    void pause();
    void cancelAll();
    void cancel(Ids::OperationId id);

    std::expected<void, Operation::Error> addDownloadOperation(
        Persistence::State const& state,
        std::string const& sshSessionOptionsKey,
        SecureShell::SftpSession& sftp,
        Ids::OperationId id,
        std::filesystem::path const& localPath,
        std::filesystem::path const& remotePath);

  private:
    std::vector<std::pair<Ids::OperationId, std::unique_ptr<Operation>>> operations_;
};
