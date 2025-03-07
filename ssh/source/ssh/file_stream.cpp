#include <ssh/file_stream.hpp>
#include <ssh/sftp_session.hpp>

#include <utility>

namespace SecureShell
{
    template <typename FunctionT>
    void FileStream::perform(FunctionT&& func)
    {
        if (auto sftp = sftp_.lock(); sftp)
            sftp->perform(std::forward<FunctionT>(func));
    }

    template <typename FunctionT>
    auto FileStream::performPromise(FunctionT&& func)
    {
        if (auto sftp = sftp_.lock(); sftp)
            return sftp->performPromise(std::forward<FunctionT>(func));

        using ResultType = std::invoke_result_t<std::decay_t<FunctionT>>;
        std::promise<ResultType> promise{};
        promise.set_value(std::unexpected(SftpError{
            .message = "Owner is null",
            .wrapperError = WrapperErrors::OwnerNull,
        }));
        return promise.get_future();
    }

    FileStream::FileStream(std::shared_ptr<SftpSession> sftp, sftp_file file, sftp_limits_struct limits)
        : sftp_{std::move(sftp)}
        , file_{file, makeFileDeleter()}
        , limits_{limits}
    {}
    FileStream::~FileStream() = default;
    FileStream::FileStream(FileStream&& other)
        : sftp_{std::move(other.sftp_)}
        , file_{other.file_.release(), makeFileDeleter()}
        , limits_{other.limits_}
    {}
    void FileStream::close()
    {
        if (auto sftp = sftp_.lock(); sftp)
        {
            sftp->fileStreamRemoveItself(this);
        }
    }
    std::function<void(sftp_file)> FileStream::makeFileDeleter()
    {
        return [this](sftp_file file) {
            if (auto sftp = this->sftp_.lock(); sftp)
            {
                // This is safe because file is just a pointer, even if 'this' is deleted by this point.
                sftp->perform([file]() {
                    sftp_close(file);
                });
            }
        };
    }
    FileStream& FileStream::operator=(FileStream&& other)
    {
        if (this != &other)
        {
            sftp_ = std::move(other.sftp_);
            file_ = std::unique_ptr<sftp_file_struct, std::function<void(sftp_file)>>{
                other.file_.release(), makeFileDeleter()};
        }
        return *this;
    }
    std::future<std::expected<void, SftpError>> FileStream::seek(std::size_t pos)
    {
        return performPromise([this, pos]() -> std::expected<void, SftpError> {
            sftp_seek64(file_.get(), pos);
            return {};
        });
    }
    std::future<std::expected<std::size_t, SftpError>> FileStream::tell()
    {
        return performPromise([this]() -> std::expected<std::size_t, SftpError> {
            return static_cast<std::size_t>(sftp_tell64(file_.get()));
        });
    }
    std::future<std::expected<void, SftpError>> FileStream::rewind()
    {
        return performPromise([this]() -> std::expected<void, SftpError> {
            sftp_rewind(file_.get());
            return {};
        });
    }
    SftpError FileStream::lastError() const
    {
        if (auto sftp = sftp_.lock(); sftp)
        {
            return sftp->lastError();
        }
        else
        {
            return SftpError{
                .message = "shared_ptr expired",
                .wrapperError = WrapperErrors::SharedPtrDestroyed,
            };
        }
    }
    std::future<std::expected<std::size_t, SftpError>> FileStream::read(std::byte* buffer, std::size_t bufferSize)
    {
        return performPromise([this, buffer, bufferSize]() -> std::expected<std::size_t, SftpError> {
            const auto result = sftp_read(file_.get(), buffer, bufferSize);
            if (result < 0)
                return std::unexpected(lastError());
            return static_cast<std::size_t>(result);
        });
    }

    std::future<std::expected<std::size_t, SftpError>>
    FileStream::read(std::function<bool(std::string_view data)> onReadFunc)
    {
        struct ReadState : public std::enable_shared_from_this<ReadState>
        {
            FileStream& stream;
            std::string buffer;
            std::function<bool(std::string_view data)> callback;
            std::promise<std::expected<std::size_t, SftpError>> promise;

            ReadState(FileStream& stream, std::size_t limit, std::function<bool(std::string_view data)> callback)
                : stream{stream}
                , buffer(std::min(4096ull, limit), '\0')
                , callback{std::move(callback)}
                , promise{}
            {}

            void onRead(std::make_signed_t<std::size_t> amount)
            {
                if (amount < 0)
                {
                    promise.set_value(std::unexpected(stream.lastError()));
                    callback({});
                    return;
                }

                if (amount == 0)
                {
                    promise.set_value(static_cast<std::size_t>(buffer.size()));
                    callback({});
                    return;
                }

                if (!callback({buffer.data(), static_cast<std::size_t>(amount)}))
                {
                    promise.set_value(static_cast<std::size_t>(buffer.size()));
                    return;
                }

                doRead();
            }

            void doRead()
            {
                stream.perform([state = shared_from_this()]() {
                    state->onRead(sftp_read(state->stream.file_.get(), state->buffer.data(), state->buffer.size()));
                });
            }
        };

        auto state = std::make_shared<ReadState>(*this, readLengthLimit(), std::move(onReadFunc));
        state->doRead();
        return state->promise.get_future();
    }
    std::size_t FileStream::writeLengthLimit() const
    {
        return limits_.max_write_length;
    }
    std::size_t FileStream::readLengthLimit() const
    {
        return limits_.max_read_length;
    }
    void FileStream::writePart(
        std::string_view toWrite,
        std::function<void(std::expected<void, SftpError>&&)> onWriteComplete)
    {
        perform([this, toWrite, onWriteComplete = std::move(onWriteComplete)]() {
            const auto written = sftp_write(file_.get(), toWrite.data(), std::min(toWrite.size(), writeLengthLimit()));

            if (written < 0)
                return onWriteComplete(std::unexpected(lastError()));

            if (static_cast<std::size_t>(written) == toWrite.size())
                return onWriteComplete({});

            if (written == 0 && !toWrite.empty())
            {
                return onWriteComplete(std::unexpected(SftpError{
                    .message = "Failed to write any data",
                    .wrapperError = WrapperErrors::ShortWrite,
                }));
            }

            writePart(toWrite.substr(written), std::move(onWriteComplete));
        });
    }
    std::future<std::expected<void, SftpError>> FileStream::write(std::string_view data)
    {
        // Short easy path:
        if (data.size() <= writeLengthLimit())
        {
            return performPromise([this, data]() -> std::expected<void, SftpError> {
                const auto written = sftp_write(file_.get(), data.data(), data.size());
                if (written < 0)
                    return std::unexpected(lastError());
                return {};
            });
        }

        // Write in chunks for large data:
        auto promise = std::make_shared<std::promise<std::expected<void, SftpError>>>();
        writePart(data, [promise](std::expected<void, SftpError>&& result) {
            promise->set_value(std::move(result));
        });
        return promise->get_future();
    }
    sftp_file FileStream::release()
    {
        sftp_.reset();
        limits_ = {};
        return file_.release();
    }
}