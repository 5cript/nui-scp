#include <ssh/processing_thread.hpp>
#include <persistence/state/terminal_engine.hpp>
#include <ssh/channel.hpp>

#include <libssh/libsshpp.hpp>

#include <expected>
#include <unordered_map>
#include <string>
#include <optional>
#include <memory>
#include <vector>
#include <future>

namespace SecureShell
{
    class Session
    {
      public:
        friend class Channel;
        friend class SftpSession;

        Session();
        ~Session();
        Session(Session const&) = delete;
        Session& operator=(Session const&) = delete;
        Session(Session&&) = delete;
        Session& operator=(Session&&) = delete;

        operator ssh::Session&()
        {
            return session_;
        }

        /**
         * @brief Starts processing the session and channels.
         *
         */
        void start();

        /**
         * @brief Stops processing but does not close the session and channels.
         */
        void stop();

        /**
         * @brief Returns true if the session is running and processing channels.
         *
         * @return true
         * @return false
         */
        bool isRunning() const;

        struct PtyCreationOptions
        {
            std::optional<std::unordered_map<std::string, std::string>> environment = std::nullopt;
            std::string terminalType = "xterm-256color";
            int columns = 80;
            int rows = 24;
            bool requestShell = true;
        };
        /**
         * @brief Creates a new channel as a pty.
         *
         * @return std::expected<ChannelId, int> The channel id or an error code
         */
        std::future<std::expected<std::weak_ptr<Channel>, int>> createPtyChannel(PtyCreationOptions options);

      private:
        void channelRemoveItself(Channel* channel);
        void removeAllChannels();

        void removalTask();

        /**
         * @brief Shuts down the session and closes all channels.
         * The session is not usable after this.
         */
        void shutdown();

      private:
        SecureShell::ProcessingThread processingThread_;
        ssh::Session session_;
        std::vector<std::shared_ptr<Channel>> channels_;
        std::vector<std::shared_ptr<Channel>> channelsToRemove_;
    };

    using AskPassCallback = int (*)(char const* prompt, char* buf, std::size_t length, int, int, void* userdata);

    struct PasswordCacheEntry
    {
        std::optional<std::string> user;
        std::string host;
        std::optional<int> port;
        std::optional<std::string> password;
    };

    std::expected<std::unique_ptr<Session>, std::string> makeSession(
        Persistence::SshTerminalEngine const& engine,
        AskPassCallback askPass,
        void* askPassUserDataKeyPhrase,
        void* askPassUserDataPassword,
        std::vector<PasswordCacheEntry>* pwCache);
}