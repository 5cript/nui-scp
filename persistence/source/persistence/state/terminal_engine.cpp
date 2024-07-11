#include <persistence/state/terminal_engine.hpp>

#include <utility/visit_overloaded.hpp>
#include <nlohmann/json.hpp>

namespace Persistence
{
    void to_json(nlohmann::json& j, BaseTerminalEngine const& engine)
    {
        j["isPty"] = engine.isPty;
    }
    void from_json(nlohmann::json const& j, BaseTerminalEngine& engine)
    {
        if (j.contains("isPty"))
            engine.isPty = j.at("isPty").get<bool>();
    }

    void to_json(nlohmann::json& j, ExecutingTerminalEngine const& engine)
    {
        j = {
            {"command", engine.command},
        };
        to_json(j, static_cast<BaseTerminalEngine const&>(engine));

        TO_JSON_OPTIONAL(j, engine, arguments);
        TO_JSON_OPTIONAL(j, engine, environment);
        TO_JSON_OPTIONAL(j, engine, exitTimeoutSeconds);
        TO_JSON_OPTIONAL(j, engine, cleanEnvironment);
    }
    void from_json(nlohmann::json const& j, ExecutingTerminalEngine& engine)
    {
        engine = {};
        from_json(j, static_cast<BaseTerminalEngine&>(engine));

        if (j.contains("command"))
            engine.command = j.at("command").get<std::string>();

        FROM_JSON_OPTIONAL(j, engine, arguments);
        FROM_JSON_OPTIONAL(j, engine, environment);
        FROM_JSON_OPTIONAL(j, engine, exitTimeoutSeconds);
        FROM_JSON_OPTIONAL(j, engine, cleanEnvironment);
    }

    void to_json(nlohmann::json& j, SshTerminalEngine const& engine)
    {
        j = {
            {"host", engine.host},
        };
        to_json(j, static_cast<BaseTerminalEngine const&>(engine));

        TO_JSON_OPTIONAL(j, engine, port);
        TO_JSON_OPTIONAL(j, engine, user);
        TO_JSON_OPTIONAL(j, engine, bindAddr);
        TO_JSON_OPTIONAL(j, engine, sshKey);
        TO_JSON_OPTIONAL(j, engine, shell);
        TO_JSON_OPTIONAL(j, engine, sshDirectory);
        TO_JSON_OPTIONAL(j, engine, knownHostsFile);
        TO_JSON_OPTIONAL(j, engine, tryAgentForAuthentication);
        TO_JSON_OPTIONAL(j, engine, logVerbosity);
        TO_JSON_OPTIONAL(j, engine, connectTimeoutSeconds);
        TO_JSON_OPTIONAL(j, engine, connectTimeoutUSeconds);
        TO_JSON_OPTIONAL(j, engine, keyExchangeAlgorithms);
        TO_JSON_OPTIONAL(j, engine, compressionClientToServer);
        TO_JSON_OPTIONAL(j, engine, compressionServerToClient);
        TO_JSON_OPTIONAL(j, engine, compressionLevel);
        TO_JSON_OPTIONAL(j, engine, strictHostKeyCheck);
        TO_JSON_OPTIONAL(j, engine, proxyCommand);
        TO_JSON_OPTIONAL(j, engine, gssapiServerIdentity);
        TO_JSON_OPTIONAL(j, engine, gssapiClientIdentity);
        TO_JSON_OPTIONAL(j, engine, gssapiDelegateCredentials);
        TO_JSON_OPTIONAL(j, engine, noDelay);
        TO_JSON_OPTIONAL(j, engine, bypassConfig);
        TO_JSON_OPTIONAL(j, engine, identityAgent);
        TO_JSON_OPTIONAL(j, engine, usePublicKeyAutoAuth);
    }
    void from_json(nlohmann::json const& j, SshTerminalEngine& engine)
    {
        engine = {};
        from_json(j, static_cast<BaseTerminalEngine&>(engine));

        if (j.contains("host"))
            engine.host = j.at("host").get<std::string>();

        FROM_JSON_OPTIONAL(j, engine, port);
        FROM_JSON_OPTIONAL(j, engine, user);
        FROM_JSON_OPTIONAL(j, engine, bindAddr);
        FROM_JSON_OPTIONAL(j, engine, sshKey);
        FROM_JSON_OPTIONAL(j, engine, shell);
        FROM_JSON_OPTIONAL(j, engine, sshDirectory);
        FROM_JSON_OPTIONAL(j, engine, tryAgentForAuthentication);
        FROM_JSON_OPTIONAL(j, engine, knownHostsFile);
        FROM_JSON_OPTIONAL(j, engine, logVerbosity);
        FROM_JSON_OPTIONAL(j, engine, connectTimeoutSeconds);
        FROM_JSON_OPTIONAL(j, engine, connectTimeoutUSeconds);
        FROM_JSON_OPTIONAL(j, engine, keyExchangeAlgorithms);
        FROM_JSON_OPTIONAL(j, engine, compressionClientToServer);
        FROM_JSON_OPTIONAL(j, engine, compressionServerToClient);
        FROM_JSON_OPTIONAL(j, engine, compressionLevel);
        FROM_JSON_OPTIONAL(j, engine, strictHostKeyCheck);
        FROM_JSON_OPTIONAL(j, engine, proxyCommand);
        FROM_JSON_OPTIONAL(j, engine, gssapiServerIdentity);
        FROM_JSON_OPTIONAL(j, engine, gssapiClientIdentity);
        FROM_JSON_OPTIONAL(j, engine, gssapiDelegateCredentials);
        FROM_JSON_OPTIONAL(j, engine, noDelay);
        FROM_JSON_OPTIONAL(j, engine, bypassConfig);
        FROM_JSON_OPTIONAL(j, engine, identityAgent);
        FROM_JSON_OPTIONAL(j, engine, usePublicKeyAutoAuth);
    }

    void to_json(nlohmann::json& j, TerminalEngine const& engine)
    {
        j = {
            {"type", engine.type},
            {"name", engine.name},
            {"options", engine.options},
            {"termiosInherit", engine.termiosInherit},
        };

        Utility::visitOverloaded(
            engine.engine,
            [&j](ExecutingTerminalEngine const& engine) {
                j["engine"] = engine;
            },
            [&j](SshTerminalEngine const& engine) {
                j["engine"] = engine;
            },
            [&j](std::monostate) {
                j["engine"] = nullptr;
            });
    }
    void from_json(nlohmann::json const& j, TerminalEngine& engine)
    {
        engine = {};

        if (j.contains("type"))
            engine.type = j.at("type").get<std::string>();

        if (j.contains("name"))
            engine.name = j.at("name").get<std::string>();

        if (j.contains("termiosInherit"))
            engine.termiosInherit = j.at("termiosInherit").get<std::string>();

        if (j.contains("options"))
            engine.options = j.at("options").get<CommonTerminalOptions>();

        if (engine.type == "shell")
        {
            ExecutingTerminalEngine e;
            from_json(j.at("engine"), e);
            engine.engine = e;
        }
        else if (engine.type == "ssh")
        {
            SshTerminalEngine e;
            from_json(j.at("engine"), e);
            engine.engine = e;
        }
        else
        {
            engine.engine = std::monostate{};
        }
    }

    ExecutingTerminalEngine defaultMsys2TerminalEngine()
    {
        ExecutingTerminalEngine engine{};
        engine.command = "C:/msys64/usr/bin/bash.exe";
        engine.arguments = {std::vector<std::string>{"--login", "-i"}};
        engine.environment = {std::unordered_map<std::string, std::string>{
            {"MSYSTEM", "MSYS"},
            {"CHERE_INVOKING", "1"},
            {"TERM", "xterm-256color"},
        }};
        engine.exitTimeoutSeconds = 3;
        return engine;
    }

    ExecutingTerminalEngine defaultBashTerminalEngine()
    {
        ExecutingTerminalEngine engine{};
        engine.command = "/bin/bash";
        engine.arguments = {std::vector<std::string>{"-i"}};
        engine.environment = {std::unordered_map<std::string, std::string>{
            {"TERM", "xterm-256color"},
        }};
        engine.exitTimeoutSeconds = 3;
        return engine;
    }
}