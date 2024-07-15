#include <persistence/state/ssh_session_options.hpp>

namespace Persistence
{
    void SshSessionOptions::useDefaultsFrom(SshSessionOptions const& other)
    {
        if (host.empty())
            host = other.host;
        if (!port.has_value())
            port = other.port;
        if (!user.has_value())
            user = other.user;
        if (!sshKey.has_value())
            sshKey = other.sshKey;

        sshOptions.useDefaultsFrom(other.sshOptions.value());
    }

    void to_json(nlohmann::json& j, SshSessionOptions const& options)
    {
        j = {{"host", options.host}};

        TO_JSON_OPTIONAL(j, options, port);
        TO_JSON_OPTIONAL(j, options, user);
        TO_JSON_OPTIONAL(j, options, sshKey);
        TO_JSON_OPTIONAL(j, options, environment);
        j["sshOptions"] = options.sshOptions;
    }
    void from_json(nlohmann::json const& j, SshSessionOptions& options)
    {
        options = {};

        FROM_JSON_OPTIONAL(j, options, port);
        FROM_JSON_OPTIONAL(j, options, user);
        FROM_JSON_OPTIONAL(j, options, sshKey);
        if (j.contains("host"))
            j.at("host").get_to(options.host);
        if (j.contains("sshOptions"))
            j.at("sshOptions").get_to(options.sshOptions);
    }
}