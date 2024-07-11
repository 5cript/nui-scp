#pragma once

#include <string>
#include <functional>
#include <optional>

class PasswordProvider
{
  public:
    virtual void getPassword(
        std::string const& whatFor,
        std::string const& prompt,
        std::function<void(std::optional<std::string>)> const& onPasswordReady) = 0;
    virtual ~PasswordProvider() = default;
};