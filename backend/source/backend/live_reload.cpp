#include <backend/live_reload.hpp>

#include <efsw/efsw.hpp>

#include <functional>
#include <string>
#include <vector>
#include <fstream>

class DynamicWatcher : public efsw::FileWatchListener
{
  public:
    DynamicWatcher(std::function<void(
                       efsw::WatchID watchid,
                       const std::string& dir,
                       const std::string& filename,
                       efsw::Action action,
                       std::string oldFilename)> callback)
        : callback_(std::move(callback))
    {}

    void handleFileAction(
        efsw::WatchID watchid,
        const std::string& dir,
        const std::string& filename,
        efsw::Action action,
        std::string oldFilename) override
    {
        callback_(watchid, dir, filename, action, oldFilename);
    }

  private:
    std::function<void(
        efsw::WatchID watchid,
        const std::string& dir,
        const std::string& filename,
        efsw::Action action,
        std::string oldFilename)>
        callback_;
};

struct WatchContext::Implementation
{
    Nui::RpcHub* hub;
    std::unique_ptr<efsw::FileWatcher> watcher;
    std::unique_ptr<DynamicWatcher> watcherListener;
    std::vector<efsw::WatchID> watches;

    void parseFile(std::string const& filename)
    {
        std::ifstream reader{filename, std::ios_base::binary};
        int lineCounter = 0;
        for (std::string line; std::getline(reader, line); ++lineCounter)
        {
            auto firstQuotationMark = line.find('"');
            if (firstQuotationMark == std::string::npos)
                continue;

            bool escaped = false;
            std::vector<std::string> literals;
            for (int i = firstQuotationMark + 1; i < line.size(); ++i)
            {
                if (line[i] == '"' && !escaped)
                {
                    const auto literal = line.substr(firstQuotationMark + 1, i - firstQuotationMark - 1);
                    literals.push_back(literal);
                    break;
                }
                escaped = line[i] == '\\';
            }
            if (literals.empty())
                continue;

            this->hub->call("nui::liveStyleReload", filename, lineCounter, literals);
        }
    }

    Implementation(Nui::RpcHub& hub, std::filesystem::path const& programDir, std::filesystem::path const& sourceDir)
        : hub{&hub}
        , watcher{std::make_unique<efsw::FileWatcher>()}
        , watcherListener{}
        , watchID{}
    {
        this->hub->registerFunction("livereload::ready", [this, programDir]() {
            auto fn = [this](
                          efsw::WatchID,
                          const std::string& dir,
                          const std::string& filename,
                          efsw::Action action,
                          std::string) {
                if (action == efsw::Actions::Modified)
                {
                    if (filename.ends_with(".css"))
                        this->hub->call("livereload::reloadStyles");
                    else
                    {
                        parseFile(dir + filename);
                    }
                }
                // this->hub->call("livereload::reloadStyles");
            };

            watcherListener = std::make_unique<DynamicWatcher>(fn);

            watchers.push_back(watcher->addWatch(
                (programDir / "liveload").string(), watcherListener.get(), false /* non recursive */));
            watchers.push_back(watcher->addWatch((programDir / "liveload").string(), watcherListener.get(), true));
            watcher->watch();
        });
    }
};

WatchContext::WatchContext(
    Nui::RpcHub& hub,
    std::filesystem::path const& programDir,
    std::filesystem::path const& sourceDir)
    : impl_{std::make_unique<Implementation>(hub, programDir, sourceDir)}
{}
WatchContext::~WatchContext()
{
    for (auto& watchID : impl_->watches)
        impl_->watcher->removeWatch(watchID);
}

std::unique_ptr<WatchContext> setupLiveWatch(Nui::RpcHub& hub, std::filesystem::path const& programDir)
{
    return std::make_unique<WatchContext>(hub, programDir);
}