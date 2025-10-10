#pragma once

#include <ssh/sftp_error.hpp>
#include <utility/describe.hpp>
#include <shared_data/file_operations/operation_type.hpp>
#include <ssh/async/processing_strand.hpp>
#include <log/log.hpp>

#include <ids/ids.hpp>

#include <optional>
#include <expected>
#include <mutex>

BOOST_DEFINE_ENUM_CLASS(
    OperationErrorType,
    FileExists,
    FileNotFound,
    OpenFailure,
    FileStreamExpired,
    FileStatFailed,
    SftpError,
    InvalidPath,
    RenameFailure,
    CannotSetFilePermissions,
    FutureTimeout,
    OperationNotPrepared,
    CannotFinalizeDuringRead,
    InvalidOptionsKey,
    TargetFileNotGood,
    CannotWorkCompletedOperation,
    CannotWorkFailedOperation,
    CannotWorkCanceledOperation,
    UnknownWorkState,
    InvalidOperationState,
    OperationNotPossibleOnFileType);

class Operation
{
  public:
    Operation()
        : id_{Ids::generateOperationId()}
    {}
    Operation(Operation const&) = delete;
    Operation& operator=(Operation const&) = delete;
    Operation(Operation&&) = delete;
    Operation& operator=(Operation&&) = delete;

    virtual ~Operation() = default;

    using ErrorType = OperationErrorType;

    virtual SharedData::OperationType type() const = 0;

    template <typename FunctionT>
    auto visit(FunctionT&& func) const;

    virtual SecureShell::ProcessingStrand* strand() const = 0;

    struct Error
    {
        ErrorType type;
        std::optional<SecureShell::SftpError> sftpError = std::nullopt;

        std::string toString() const
        {
            const auto enumString = boost::describe::enum_to_string(type, "INVALID_ENUM_VALUE");
            if (sftpError.has_value())
                return fmt::format("{}: {}", enumString, sftpError->toString());
            return enumString;
        }
    };

    template <typename FunctionT>
    bool perform(FunctionT&& func)
    {
        if (auto* theStrand = strand(); theStrand)
            return theStrand->pushTask(std::forward<FunctionT>(func));
        else
        {
            Log::error("Operation: Cannot perform task on strand, no processing strand available.");
            return false;
        }
    }

    Ids::OperationId id() const
    {
        return id_;
    }

    enum class OperationState
    {
        NotStarted,
        Preparing,
        Prepared,
        Running,
        Finalizing,
        Completed,
        Canceled,
        Failed,
    };

    OperationState state() const
    {
        return state_;
    }

    /**
     * @brief Can parallel actions go beyond this operation?
     *
     * @return true Cannot progress beyond this operation.
     * @return false Can progress beyond this operation.
     */
    virtual bool isBarrier() const noexcept = 0;

    /**
     * @brief How much parallel work does this operation do.
     *
     * @param parallel Maximum parallelism allowed.
     * @return The amount of parallel work that can be done maxed by parallel parameter.
     */
    virtual int parallelWorkDoable(int parallel) const noexcept = 0;

    enum class WorkStatus
    {
        MoreWork,
        Complete
    };

    /**
     * @brief Performs work for the operation depending on the operation type.
     *
     * @return std::expected<bool, Error>, true if it wants to be retriggered without delay.
     */
    virtual std::expected<WorkStatus, Error> work() = 0;

    /**
     * @brief Cancels the operation.
     *
     * @return std::expected<void, Error>
     */
    virtual std::expected<void, Error> cancel(bool adoptCancelState) = 0;

    template <typename T = void>
    std::expected<T, Error> enterErrorState(Error error)
    {
        state_ = OperationState::Failed;
        error_ = std::move(error);
        const auto cancelResult = cancel(false);
        if (!cancelResult.has_value())
        {
            Log::error("Operation: Failed to cancel operation: {}", cancelResult.error().toString());
            // If cancel fails, we still want to call the completion callback.
        }
        return std::unexpected(error_.value());
    }

  protected:
    void enterState(OperationState newState)
    {
        state_ = newState;
    }

  protected:
    OperationState state_{OperationState::NotStarted};
    std::optional<Error> error_{std::nullopt};

  private:
    Ids::OperationId id_;
};