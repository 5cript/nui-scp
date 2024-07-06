#include <persistence/state/terminal_engine.hpp>

#include <utility/visit_overloaded.hpp>

namespace Persistence
{
    void to_json(nlohmann::json& j, ExecutingTerminalEngine const& engine)
    {
        j = nlohmann::json{
            {"command", engine.command},
        };

        TO_JSON_OPTIONAL(j, engine, arguments);
        TO_JSON_OPTIONAL(j, engine, environment);
        TO_JSON_OPTIONAL(j, engine, exitTimeoutSeconds);
        TO_JSON_OPTIONAL(j, engine, cleanEnvironment);
    }
    void from_json(nlohmann::json const& j, ExecutingTerminalEngine& engine)
    {
        j.at("command").get_to(engine.command);

        FROM_JSON_OPTIONAL(j, engine, arguments);
        FROM_JSON_OPTIONAL(j, engine, environment);
        FROM_JSON_OPTIONAL(j, engine, exitTimeoutSeconds);
        FROM_JSON_OPTIONAL(j, engine, cleanEnvironment);
    }

    void to_json(nlohmann::json& j, SshTerminalEngine const& engine)
    {
        j = nlohmann::json{
            {"host", engine.host},
        };

        TO_JSON_OPTIONAL(j, engine, port);
        TO_JSON_OPTIONAL(j, engine, user);
        TO_JSON_OPTIONAL(j, engine, privateKey);
        TO_JSON_OPTIONAL(j, engine, keyPassphrase);
        TO_JSON_OPTIONAL(j, engine, shell);
        TO_JSON_OPTIONAL(j, engine, sshDirectory);
        TO_JSON_OPTIONAL(j, engine, tryAgentForAuthentication);
    }
    void from_json(nlohmann::json const& j, SshTerminalEngine& engine)
    {
        j.at("host").get_to(engine.host);

        FROM_JSON_OPTIONAL(j, engine, port);
        FROM_JSON_OPTIONAL(j, engine, user);
        FROM_JSON_OPTIONAL(j, engine, privateKey);
        FROM_JSON_OPTIONAL(j, engine, keyPassphrase);
        FROM_JSON_OPTIONAL(j, engine, shell);
        FROM_JSON_OPTIONAL(j, engine, sshDirectory);
        FROM_JSON_OPTIONAL(j, engine, tryAgentForAuthentication);
    }

    void to_json(nlohmann::json& j, TerminalEngine const& engine)
    {
        j = nlohmann::json{
            {"type", engine.type},
        };

        Utility::visitOverloaded(
            [&j](ExecutingTerminalEngine const& engine) {
                j["engine"] = engine;
            },
            [&j](SshTerminalEngine const& engine) {
                j["engine"] = engine;
            },
            engine.engine);
    }
    void from_json(nlohmann::json const& j, TerminalEngine& engine)
    {
        j.at("type").get_to(engine.type);
        Utility::visitOverloaded(
            [&j, &engine](ExecutingTerminalEngine& engine) {
                j.at("engine").get_to(engine);
            },
            [&j, &engine](SshTerminalEngine& engine) {
                j.at("engine").get_to(engine);
            },
            engine.engine);
    }

    ExecutingTerminalEngine defaultMsys2TerminalEngine()
    {
        ExecutingTerminalEngine engine;
        engine.command = "C:/msys64/usr/bin/bash.exe";
        engine.arguments = {"--login", "-i"};
        engine.environment = {
            {"MSYSTEM", "MSYS"},
            {"CHERE_INVOKING", "1"},
            {"TERM", "xterm-256color"},
        };
        return engine;
    }

    ExecutingTerminalEngine defaultBashTerminalEngine()
    {
        ExecutingTerminalEngine engine;
        engine.command = "/bin/bash";
        engine.arguments = {"-i"};
        engine.environment = {
            {"TERM", "xterm-256color"},
        };
        return engine;
    }
}