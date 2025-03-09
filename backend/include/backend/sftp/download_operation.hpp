#pragma once

#include <backend/sftp/operation.hpp>
#include <ssh/file_stream.hpp>
#include <nui/utility/move_detector.hpp>

#include <filesystem>
#include <fstream>
#include <string>

class DownloadOperation : public Operation
{
  public:
    struct DownloadOperationOptions
    {
        std::function<void(std::int64_t min, std::int64_t max, std::int64_t current)> progressCallback =
            [](auto, auto, auto) {};
        std::function<void(bool success)> onCompletionCallback = [](auto) {};
        std::filesystem::path remotePath{};
        std::filesystem::path localPath{};
        std::string tempFileSuffix{".filepart"};
        bool mayOverwrite{false};
        bool reserveSpace{false};
        bool tryContinue{false};
        bool inheritPermissions{false};
        bool doCleanup{true};
        std::optional<std::filesystem::perms> permissions{std::nullopt};
        std::chrono::seconds futureTimeout{5};
    };

    DownloadOperation(std::weak_ptr<SecureShell::IFileStream> fileStream, DownloadOperationOptions options);
    ~DownloadOperation() override;
    DownloadOperation(DownloadOperation const&) = delete;
    DownloadOperation(DownloadOperation&&) = delete;
    DownloadOperation& operator=(DownloadOperation const&) = delete;
    DownloadOperation& operator=(DownloadOperation&&) = delete;

    std::expected<void, Error> prepare() override;
    std::expected<void, Error> start() override;
    void cancel() override;
    std::expected<void, Error> pause() override;
    std::expected<void, Error> finalize() override;

  private:
    std::expected<void, Error> openOrAdoptFile(SecureShell::IFileStream& stream);

    template <typename FunctionT>
    bool perform(FunctionT&& func)
    {
        auto stream = fileStream_.lock();
        if (!stream)
            return false;

        auto* strand = stream->strand();
        if (!strand)
            return false;

        return strand->pushTask(std::forward<FunctionT>(func));
    }

    void cleanup();

  private:
    std::recursive_mutex mutex_;
    std::weak_ptr<SecureShell::IFileStream> fileStream_;
    std::filesystem::path remotePath_;
    std::filesystem::path localPath_;
    std::string tempFileSuffix_;
    std::function<void(std::int64_t min, std::int64_t max, std::int64_t current)> progressCallback_;
    std::function<void(bool success)> onCompletionCallback_;
    bool mayOverwrite_;
    bool reserveSpace_;
    bool tryContinue_;
    bool inheritPermissions_;
    bool doCleanup_;
    bool preparationDone_;
    std::atomic_bool interuptRead_;
    std::optional<std::filesystem::perms> permissions_;
    std::ofstream localFile_;
    std::uint64_t fileSize_;
    std::future<std::expected<std::size_t, SecureShell::SftpError>> readFuture_;
    std::chrono::seconds futureTimeout_;
};