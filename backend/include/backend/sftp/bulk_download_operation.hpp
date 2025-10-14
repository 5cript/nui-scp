#pragma once

#include <backend/sftp/operation.hpp>
#include <ssh/file_stream.hpp>
#include <nui/utility/move_detector.hpp>
#include <backend/sftp/download_operation.hpp>

#include <filesystem>
#include <fstream>
#include <string>

// TODO: concurrent downloads!
class BulkDownloadOperation : public Operation
{
  public:
    struct BulkDownloadOperationOptions
    {
        std::function<void(
            std::filesystem::path const& currentFile,
            std::uint64_t fileCurrentIndex,
            std::uint64_t fileCount,
            std::uint64_t bytesCurrent,
            std::uint64_t bytesTotal)>
            overallProgressCallback = [](auto const&, auto, auto, auto, auto) {};

        // TODO: as tar, do compress tar?
        DownloadOperation::DownloadOperationOptions individualOptions = {};
    };

    BulkDownloadOperation(SecureShell::SftpSession& sftp, BulkDownloadOperationOptions options);
    ~BulkDownloadOperation() override;
    BulkDownloadOperation(BulkDownloadOperation const&) = delete;
    BulkDownloadOperation(BulkDownloadOperation&&) = delete;
    BulkDownloadOperation& operator=(BulkDownloadOperation const&) = delete;
    BulkDownloadOperation& operator=(BulkDownloadOperation&&) = delete;

    std::expected<WorkStatus, Error> work() override;
    SharedData::OperationType type() const override;
    std::expected<void, Error> cancel(bool adoptCancelState) override;

    void setScanResult(std::vector<SharedData::DirectoryEntry>&& entries);

    bool isBarrier() const noexcept override
    {
        return false;
    }

    // TODO: can do more than 1.
    int parallelWorkDoable(int) const noexcept override
    {
        return 1;
    }

    SecureShell::ProcessingStrand* strand() const override;

  private:
    SecureShell::SftpSession* sftp_;
    BulkDownloadOperationOptions options_;
    std::unique_ptr<DownloadOperation> currentDownload_;
    std::vector<SharedData::DirectoryEntry> entries_;
    std::uint64_t totalBytes_{0};
    std::uint64_t currentIndex_{0};
    std::uint64_t currentBytes_{0};
    std::chrono::seconds futureTimeout_{5};
};