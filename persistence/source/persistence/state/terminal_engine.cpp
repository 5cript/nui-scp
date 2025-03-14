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
        to_json(j, static_cast<BaseTerminalEngine const&>(engine));
        j["sshSessionOptions"] = engine.sshSessionOptions;
    }
    void from_json(nlohmann::json const& j, SshTerminalEngine& engine)
    {
        engine = {};
        from_json(j, static_cast<BaseTerminalEngine&>(engine));

        if (j.contains("sshSessionOptions"))
            j.at("sshSessionOptions").get_to(engine.sshSessionOptions);
    }

    void to_json(nlohmann::json& j, TerminalEngine const& engine)
    {
        j = {
            {"type", engine.type},
            {"terminalOptions", engine.terminalOptions},
            {"termios", engine.termios},
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

        TO_JSON_OPTIONAL(j, engine, startupSession);
        TO_JSON_OPTIONAL(j, engine, orderBy);

        if (engine.layouts)
            j["layouts"] = *engine.layouts;
    }
    void from_json(nlohmann::json const& j, TerminalEngine& engine)
    {
        engine = {};

        if (j.contains("type"))
            j.at("type").get_to(engine.type);

        if (j.contains("termios"))
            j.at("termios").get_to(engine.termios);

        if (j.contains("terminalOptions"))
            j.at("terminalOptions").get_to(engine.terminalOptions);

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

        FROM_JSON_OPTIONAL(j, engine, startupSession);
        FROM_JSON_OPTIONAL(j, engine, orderBy);

        if (j.contains("layouts"))
            engine.layouts = j.at("layouts");
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