#include <backend/sftp/download_operation.hpp>

#include <log/log.hpp>

DownloadOperation::DownloadOperation(
    std::weak_ptr<SecureShell::IFileStream> fileStream,
    DownloadOperationOptions options)
    : Operation{}
    , mutex_{}
    , fileStream_{std::move(fileStream)}
    , remotePath_{std::move(options.remotePath)}
    , localPath_{std::move(options.localPath)}
    , tempFileSuffix_{std::move(options.tempFileSuffix)}
    , progressCallback_{std::move(options.progressCallback)}
    , onCompletionCallback_{[this, cb = std::move(options.onCompletionCallback), once = false](bool success) mutable {
        if (!cb)
            throw std::invalid_argument("onCompletionCallback must be set.");
        if (once)
            return;
        once = true;

        std::scoped_lock lock{mutex_};
        isReading_ = false;
        progressCallback_(0, fileSize_, fileSize_);
        cb(success);
    }}
    , mayOverwrite_{options.mayOverwrite}
    , reserveSpace_{options.reserveSpace}
    , tryContinue_{options.tryContinue}
    , inheritPermissions_{options.inheritPermissions}
    , doCleanup_{options.doCleanup}
    , preparationDone_{false}
    , isReading_{false}
    , interuptRead_{false}
    , permissions_{options.permissions}
    , localFile_{}
    , fileSize_{0}
    , readFuture_{}
    , futureTimeout_{options.futureTimeout}
{
    if (tempFileSuffix_.empty())
        tempFileSuffix_ = ".filepart";
    if (tempFileSuffix_.find('/') != 0)
        tempFileSuffix_ = ".filepart";
}

DownloadOperation::~DownloadOperation()
{
    cleanup();

    if (auto stream = fileStream_.lock(); stream)
    {
        // wait for all tasks of the operation to finish
        stream->strand()->pushPromiseTask([]() {}).get();
    }
}

std::expected<void, DownloadOperation::Error> DownloadOperation::openOrAdoptFile(SecureShell::IFileStream& stream)
{
    std::scoped_lock lock{mutex_};

    const auto tempPath = localPath_.generic_string() + tempFileSuffix_;

    if (tryContinue_ && std::filesystem::exists(tempPath))
    {
        localFile_.open(tempPath, std::ios::binary | std::ios::app);
        if (!localFile_.is_open())
        {
            Log::error("DownloadOperation: Failed to open file for appending: {}", tempPath);
            return std::unexpected(Error{.type = ErrorType::OpenFailure});
        }

        // File complete but not renamed? just rename it in the finalize() step
        if (static_cast<std::uint64_t>(localFile_.tellp()) == fileSize_)
        {
            Log::info("DownloadOperation: File '{}' already complete, will be renamed in finalize() step.", tempPath);
            localFile_.close();
            return {};
        }
        // File is larger than expected? discard it and start over.
        else if (static_cast<std::uint64_t>(localFile_.tellp()) > fileSize_)
        {
            Log::info("DownloadOperation: File '{}' is larger than expected, discarding and starting over.", tempPath);
            localFile_.close();
            // Reset the file
            localFile_.open(tempPath, std::ios::binary | std::ios::trunc);
        }
        else
        {
            Log::info("DownloadOperation: File '{}' is incomplete, continuing download.", tempPath);
            // Seek stream to position:
            auto seekResult = stream.seek(localFile_.tellp()).get();
            if (!seekResult.has_value())
                return std::unexpected(Error{.type = ErrorType::FileStatFailed});
        }
    }
    else
    {
        Log::info("DownloadOperation: Starting new download to '{}'.", tempPath);
        localFile_.open(tempPath, std::ios::binary | std::ios::trunc);
    }

    if (!localFile_.is_open())
    {
        Log::error("DownloadOperation: Failed to open file: {}", tempPath);
        return std::unexpected(Error{.type = ErrorType::OpenFailure});
    }

    return {};
}

std::expected<void, Operation::Error> DownloadOperation::prepare()
{
    std::scoped_lock lock{mutex_};

    if (localPath_.empty())
    {
        Log::error("DownloadOperation: Invalid local path.");
        return std::unexpected(Error{.type = ErrorType::InvalidPath});
    }

    // Initial check. Check again later before rename
    if (std::filesystem::exists(localPath_))
    {
        if (!mayOverwrite_)
        {
            Log::error(
                "DownloadOperation: File '{}' already exists and may not be overwritten.", localPath_.generic_string());
            return std::unexpected(Error{.type = ErrorType::FileExists});
        }
    }

    auto stream = fileStream_.lock();
    if (!stream)
    {
        Log::error("DownloadOperation: File stream expired.");
        return std::unexpected(Error{.type = ErrorType::FileStreamExpired});
    }

    const auto fileInfo = stream->stat().get();
    if (!fileInfo.has_value())
    {
        Log::error("DownloadOperation: Failed to stat file.");
        return std::unexpected(Error{.type = ErrorType::FileStatFailed, .sftpError = fileInfo.error()});
    }

    fileSize_ = fileInfo->size;

    auto openResult = openOrAdoptFile(*stream);
    if (!openResult.has_value())
    {
        Log::error("DownloadOperation: Failed to open file.");
        return std::unexpected(std::move(openResult).error());
    }

    if (reserveSpace_ && fileSize_ != 0)
    {
        // Reserve space
        Log::info("DownloadOperation: Reserving space for file.");
        const auto pos = localFile_.tellp();
        localFile_.seekp(fileSize_ - 1);
        localFile_.put('\0');
        if (localFile_.fail())
            return std::unexpected(Error{.type = ErrorType::OpenFailure});
        localFile_.seekp(pos);
    }

    Log::info(
        "DownloadOperation: Prepared download of '{}' to '{}'.",
        remotePath_.generic_string(),
        localPath_.generic_string());

    preparationDone_ = true;
    return {};
}
std::expected<void, Operation::Error> DownloadOperation::start()
{
    std::scoped_lock lock{mutex_};
    interuptRead_ = false;

    if (!preparationDone_)
    {
        Log::error("DownloadOperation: Operation not prepared.");
        return std::unexpected(Error{.type = ErrorType::OperationNotPrepared});
    }

    if (!localFile_.is_open())
    {
        Log::error("DownloadOperation: File is not open.");
        return std::unexpected(Error{.type = ErrorType::OpenFailure});
    }

    if (fileSize_ == 0)
    {
        Log::info("DownloadOperation: Remote file is empty, nothing to do.");
        return {};
    }

    auto stream = fileStream_.lock();
    if (!stream)
    {
        Log::error("DownloadOperation: File stream expired.");
        return std::unexpected(Error{.type = ErrorType::FileStreamExpired});
    }

    isReading_ = true;
    readFuture_ = stream->read([this](std::string_view data) {
        if (data.size() == 0)
        {
            {
                std::lock_guard lock{mutex_};
                isReading_ = false;
            }
            Log::info("DownloadOperation: Remote file read complete or error.");
            // Defer completion by pushing it onto the strand.
            perform([this]() {
                std::scoped_lock lock{mutex_};
                onCompletionCallback_(true);
            });
            return true;
        }

        std::uint64_t fileSize = 0;
        std::uint64_t tellp = 0;
        bool doContinue = true;
        bool good = true;
        {
            std::scoped_lock lock{mutex_};
            localFile_.write(data.data(), data.size());
            fileSize = fileSize_;
            tellp = static_cast<uint64_t>(localFile_.tellp());
            good = localFile_.good();
            doContinue = good && !interuptRead_;
            isReading_ = doContinue;
        }
        progressCallback_(0, fileSize, tellp);
        if (!doContinue)
        {
            if (!good)
            {
                Log::error("DownloadOperation read cycle stopped: localFile_.good() == false");
                perform([this]() {
                    std::scoped_lock lock{mutex_};
                    onCompletionCallback_(false);
                });
            }
        }
        return doContinue;
    });

    return {};
}
std::expected<void, DownloadOperation::Error> DownloadOperation::cancel()
{
    {
        std::scoped_lock lock{mutex_};

        if (isReading_)
        {
            const auto res = pause();
            if (!res.has_value())
            {
                Log::error("DownloadOperation: Failed to pause download.");
                return res;
            }
        }

        Log::info(
            "DownloadOperation: Download of '{}' to '{}' canceled.",
            remotePath_.generic_string(),
            localPath_.generic_string());
    }

    perform([this]() {
        std::scoped_lock lock{mutex_};
        onCompletionCallback_(false);
    });

    cleanup();

    return {};
}
void DownloadOperation::cleanup()
{
    std::scoped_lock lock{mutex_};
    localFile_.close();

    if (doCleanup_ && std::filesystem::exists(localPath_.generic_string() + tempFileSuffix_))
        std::filesystem::remove(localPath_.generic_string() + tempFileSuffix_);

    if (auto stream = fileStream_.lock(); stream)
        stream->close(false);
}
std::expected<void, DownloadOperation::Error> DownloadOperation::pause()
{
    {
        std::scoped_lock lock{mutex_};
        interuptRead_ = true;
    }

    if (readFuture_.wait_for(futureTimeout_) != std::future_status::ready)
    {
        Log::error("DownloadOperation: Failed to pause download.");
        return std::unexpected(Error{.type = ErrorType::FutureTimeout});
    }
    {
        std::scoped_lock lock{mutex_};
        isReading_ = false;
    }
    return {};
}
std::expected<void, DownloadOperation::Error> DownloadOperation::finalize()
{
    std::scoped_lock lock{mutex_};

    if (isReading_)
    {
        Log::error("DownloadOperation: Cannot finalize while reading.");
        return std::unexpected(Error{.type = ErrorType::CannotFinalizeDuringRead});
    }

    if (readFuture_.wait_for(futureTimeout_) != std::future_status::ready)
    {
        Log::error("DownloadOperation: Failed to wait for download completion.");
        return std::unexpected(Error{.type = ErrorType::FutureTimeout});
    }

    localFile_.close();

    if (std::filesystem::exists(localPath_) && !mayOverwrite_)
    {
        Log::error(
            "DownloadOperation: File '{}' already exists and may not be overwritten.", localPath_.generic_string());
        return std::unexpected(Error{.type = ErrorType::FileExists});
    }

    std::error_code ec{};
    std::filesystem::rename(localPath_.generic_string() + tempFileSuffix_, localPath_, ec);
    if (ec)
    {
        Log::error("DownloadOperation: Failed to rename file: {}", ec.message());
        return std::unexpected(Error{.type = ErrorType::RenameFailure});
    }

    if (inheritPermissions_)
    {
        Log::info("DownloadOperation: Inheriting permissions from remote file.");
        auto stream = fileStream_.lock();
        if (!stream)
        {
            Log::error("DownloadOperation: File stream expired.");
            return std::unexpected(Error{.type = ErrorType::FileStreamExpired});
        }

        const auto fileInfo = stream->stat().get();
        if (!fileInfo.has_value())
        {
            Log::error("DownloadOperation: Failed to stat file.");
            return std::unexpected(Error{.type = ErrorType::FileStatFailed, .sftpError = fileInfo.error()});
        }

        std::error_code permissionsError{};
        std::filesystem::permissions(localPath_, fileInfo->permissions, permissionsError);
        if (permissionsError)
        {
            Log::error("DownloadOperation: Failed to set permissions: {}", permissionsError.message());
            return std::unexpected(Error{.type = ErrorType::CannotSetFilePermissions});
        }
    }
    else if (permissions_)
    {
        Log::info("DownloadOperation: Setting permissions.");
        std::error_code permissionsError{};
        std::filesystem::permissions(localPath_, *permissions_, permissionsError);
        if (permissionsError)
        {
            Log::error("DownloadOperation: Failed to set permissions: {}", permissionsError.message());
            return std::unexpected(Error{.type = ErrorType::CannotSetFilePermissions});
        }
    }

    Log::info(
        "DownloadOperation: Finalized download of '{}' to '{}'.",
        remotePath_.generic_string(),
        localPath_.generic_string());
    return {};
}