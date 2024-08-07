#pragma once

#include <string_view>
#include <functional>

class TerminalEngine
{
  public:
    virtual void open(std::function<void(bool, std::string const&)> onOpen) = 0;
    virtual void dispose(/* parameters? */) = 0;
    virtual void write(std::string const& data) = 0;
    virtual void resize(int cols, int rows) = 0;
    virtual ~TerminalEngine() = default;

    virtual void setStdoutHandler(std::function<void(std::string const&)> handler) = 0;
    virtual void setStderrHandler(std::function<void(std::string const&)> handler) = 0;
};