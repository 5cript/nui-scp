#include <backend/process/boost_process.hpp>

#include <backend/process/environment.hpp>

namespace bp2 = boost::process::v2;

Environment::Environment(
    bool clean,
    std::unordered_map<std::string, std::string> const& mergeIn,
    std::string const& pathMergeIn)
    : environment_{}
{
    if (!clean)
        loadFromCurrent();

    merge(mergeIn, true);
    extendPath(pathMergeIn);
}
std::unordered_map<std::string, std::string>& Environment::environment()
{
    return environment_;
}
void Environment::loadFromCurrent()
{
    environment_ = {};
    const auto currentEnv = bp2::environment::current();
    for (auto iter = currentEnv.begin(); iter != currentEnv.end(); ++iter)
    {
        auto deref = *iter;
        environment_.emplace(
            [&]() {
                if (deref.key().size() > 0)
                    return deref.key().string();
                return std::string{};
            }(),
            [&]() {
                if (deref.value().size() > 0)
                    return deref.value().string();
                return std::string{};
            }());
    }
}
void Environment::extendPath(std::string const& path, bool front)
{
    auto iter = environment_.find("PATH");
    if (iter == environment_.end())
    {
        environment_["PATH"] = path;
    }
    else
    {
        if (front)
            iter->second = path + ":" + iter->second;
        else
            iter->second += ":" + path;
    }
}
void Environment::merge(std::unordered_map<std::string, std::string> const& other, bool overwrite)
{
    for (auto const& [key, value] : other)
    {
        if (overwrite || environment_.find(key) == environment_.end())
        {
            environment_[key] = value;
        }
    }
}