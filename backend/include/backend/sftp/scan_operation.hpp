#pragma once

#include <ssh/file_stream.hpp>
#include <backend/sftp/operation.hpp>
#include <nui/utility/move_detector.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <cstdint>

class ScanOperation : public Operation
{
  public:
    struct ScanOperationOptions
    {
        std::function<void(std::uint64_t totalBytes, std::uint64_t currentIndex, std::uint64_t totalScanned)>
            progressCallback = [](auto, auto, auto) {};
        std::filesystem::path remotePath{};
        std::chrono::seconds futureTimeout{5};
    };

    SecureShell::ProcessingStrand* strand() const override;

    ScanOperation(SecureShell::SftpSession& sftp, ScanOperationOptions options);
    ~ScanOperation() override;
    ScanOperation(ScanOperation const&) = delete;
    ScanOperation(ScanOperation&&) = delete;
    ScanOperation& operator=(ScanOperation const&) = delete;
    ScanOperation& operator=(ScanOperation&&) = delete;

    std::expected<WorkStatus, Error> work() override;

    SharedData::OperationType type() const override
    {
        return SharedData::OperationType::Scan;
    }

    bool isBarrier() const noexcept override
    {
        return true;
    }

    int parallelWorkDoable(int) const noexcept override
    {
        return 1;
    }

    std::filesystem::path remotePath() const
    {
        return remotePath_;
    }

    std::expected<void, Error> cancel(bool adoptCancelState) override;

    /**
     * @brief Eject the scanned directory entries. Careful!: The internal list is moved out.
     *
     * @return std::vector<SharedData::DirectoryEntry>
     */
    std::vector<SharedData::DirectoryEntry> ejectEntries()
    {
        return std::move(entries_);
    }

  private:
    std::expected<void, ScanOperation::Error> scanOnce(std::filesystem::path const& path);

  private:
    SecureShell::SftpSession* sftp_;
    std::filesystem::path remotePath_;
    std::function<void(std::uint64_t totalBytes, std::uint64_t currentIndex, std::uint64_t totalScanned)>
        progressCallback_;
    std::chrono::seconds futureTimeout_;
    std::vector<SharedData::DirectoryEntry> entries_{};
    std::uint64_t currentIndex_{0};
    std::uint64_t totalBytes_{0};
};