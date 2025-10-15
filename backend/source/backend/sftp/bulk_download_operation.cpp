#include <backend/sftp/bulk_download_operation.hpp>
#include <ssh/sftp_session.hpp>
#include <log/log.hpp>

BulkDownloadOperation::BulkDownloadOperation(SecureShell::SftpSession& sftp, BulkDownloadOperationOptions options)
    : Operation{}
    , sftp_{&sftp}
    , options_{std::move(options)}
    , currentDownload_{nullptr}
    , entries_{}
    , totalBytes_{0}
    , currentIndex_{0}
    , currentBytes_{0}
    , futureTimeout_{options_.individualOptions.futureTimeout}
{}

BulkDownloadOperation::~BulkDownloadOperation() = default;

std::expected<BulkDownloadOperation::WorkStatus, BulkDownloadOperation::Error> BulkDownloadOperation::work()
{
    using enum OperationState;

    // TODO: Not implemented yet

    return WorkStatus::Complete;
}

SharedData::OperationType BulkDownloadOperation::type() const
{
    return SharedData::OperationType::BulkDownload;
}

void BulkDownloadOperation::setScanResult(std::vector<SharedData::DirectoryEntry>&& entries)
{
    entries_ = std::move(entries);
    totalBytes_ = 0;
}

std::expected<void, BulkDownloadOperation::Error> BulkDownloadOperation::cancel(bool adoptCancelState)
{
    if (adoptCancelState)
        enterState(OperationState::Canceled);
    return {};
}

SecureShell::ProcessingStrand* BulkDownloadOperation::strand() const
{
    return sftp_->strand();
}