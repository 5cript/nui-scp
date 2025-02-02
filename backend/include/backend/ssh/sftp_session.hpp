#pragma once

#include <libssh/sftp.h>
#include <nui/utility/move_detector.hpp>
#include <shared_data/directory_entry.hpp>

#include <expected>
#include <vector>
#include <optional>

class Session;

class SftpSession
{
  public:
    struct Error
    {
        std::string message;
        int sshError = 0;
        int sftpError = 0;
    };
    friend std::expected<SftpSession, Error> makeSftpSession(Session& session);

    ~SftpSession();
    SftpSession(SftpSession const&) = delete;
    SftpSession& operator=(SftpSession const&) = delete;
    SftpSession(SftpSession&&) = default;
    SftpSession& operator=(SftpSession&&) = default;

    struct DirectoryEntry : public SharedData::DirectoryEntry
    {
        static DirectoryEntry fromSftpAttributes(sftp_attributes attributes);
    };

    std::expected<std::vector<DirectoryEntry>, Error> listDirectory(std::filesystem::path const& path);
    std::optional<Error> createDirectory(std::filesystem::path const& path);

    void disconnect();

  private:
    SftpSession(sftp_session session);

  private:
    sftp_session session_;
    Nui::MoveDetector moveDetector_;
};

std::expected<SftpSession, SftpSession::Error> makeSftpSession(Session& session);