#pragma once

#include <ssh/sftp_error.hpp>
#include <utility/describe.hpp>

#include <ids/ids.hpp>

#include <optional>
#include <expected>

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
    InvalidOptionsKey);

class Operation
{
  public:
    Operation()
        : id_{Ids::generateOperationId()}
    {}
    Operation(Operation const&) = delete;
    Operation& operator=(Operation const&) = delete;
    Operation(Operation&&) = default;
    Operation& operator=(Operation&&) = default;

    using ErrorType = OperationErrorType;

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

    Ids::OperationId id() const
    {
        return id_;
    }

    virtual ~Operation() = default;

    virtual std::expected<void, Error> prepare() = 0;

    /**
     * @brief Starts the operation, can also be used to resume cancelled operations.
     *
     * @return std::expected<void, Error>
     */
    virtual std::expected<void, Error> start() = 0;

    /**
     * @brief Cancels the operation and possibly does some cleanup.
     */
    virtual std::expected<void, Error> cancel() = 0;

    /**
     * @brief Pauses the operation.
     */
    virtual std::expected<void, Error> pause() = 0;

    /**
     * @brief Finalizes the operation. This should be called after the operation is done (progress == max).
     */
    virtual std::expected<void, Error> finalize() = 0;

  private:
    Ids::OperationId id_;
};