#include <frontend/terminal/file_engine.hpp>
#include <frontend/terminal/ssh_engine.hpp>
#include <nui/utility/move_detector.hpp>
#include <roar/detail/pimpl_special_functions.hpp>

class SftpFileEngine : public FileEngine
{
  public:
    SftpFileEngine(SshTerminalEngine* engine);
    ROAR_PIMPL_SPECIAL_FUNCTIONS(SftpFileEngine);

    void listDirectory(
        std::filesystem::path const& path,
        std::function<void(std::optional<std::vector<SharedData::DirectoryEntry>> const&)> onComplete) override;
    void dispose() override;
    void createDirectory(std::filesystem::path const& path, std::function<void(bool)> onComplete) override;
    void createFile(std::filesystem::path const& path, std::function<void(bool)> onComplete) override;

    std::optional<Ids::ChannelId> release();

  private:
    void lazyOpen(std::function<void(std::optional<Ids::ChannelId> const&)> const& onOpen);

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
    Nui::MoveDetector moveDetector_;
};