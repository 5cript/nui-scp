#pragma once

#include <unordered_map>
#include <string>

class Environment
{
  public:
    Environment() = default;
    Environment(
        bool clean,
        std::unordered_map<std::string, std::string> const& mergeIn,
        std::string const& pathMergeIn = {});

    std::unordered_map<std::string, std::string>& environment();
    void loadFromCurrent();
    void extendPath(std::string const& path, bool front = true);
    void merge(std::unordered_map<std::string, std::string> const& other, bool overwrite = true);

  private:
    std::unordered_map<std::string, std::string> environment_;
};