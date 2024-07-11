#include <backend/password/password_prompter.hpp>

PasswordPrompter::PasswordPrompter(Nui::RpcHub& rpcHub)
    : rpcHub_(&rpcHub)
{}

void PasswordPrompter::getPassword(
    std::string const& whatFor,
    std::string const& prompt,
    std::function<void(std::optional<std::string>)> const& onPasswordReady)
{
    // TODO:
    onPasswordReady(std::nullopt);
}