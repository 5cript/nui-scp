#include <persistence/state/ssh_options.hpp>

namespace Persistence
{
    void to_json(nlohmann::json& j, SshOptions const& options)
    {
        TO_JSON_OPTIONAL(j, options, bindAddr);
        TO_JSON_OPTIONAL(j, options, sshKey);
        TO_JSON_OPTIONAL(j, options, sshDirectory);
        TO_JSON_OPTIONAL(j, options, tryAgentForAuthentication);
        TO_JSON_OPTIONAL(j, options, knownHostsFile);
        TO_JSON_OPTIONAL(j, options, logVerbosity);
        TO_JSON_OPTIONAL(j, options, connectTimeoutSeconds);
        TO_JSON_OPTIONAL(j, options, connectTimeoutUSeconds);
        TO_JSON_OPTIONAL(j, options, keyExchangeAlgorithms);
        TO_JSON_OPTIONAL(j, options, compressionClientToServer);
        TO_JSON_OPTIONAL(j, options, compressionServerToClient);
        TO_JSON_OPTIONAL(j, options, compressionLevel);
        TO_JSON_OPTIONAL(j, options, strictHostKeyCheck);
        TO_JSON_OPTIONAL(j, options, proxyCommand);
        TO_JSON_OPTIONAL(j, options, gssapiServerIdentity);
        TO_JSON_OPTIONAL(j, options, gssapiClientIdentity);
        TO_JSON_OPTIONAL(j, options, gssapiDelegateCredentials);
        TO_JSON_OPTIONAL(j, options, noDelay);
        TO_JSON_OPTIONAL(j, options, bypassConfig);
        TO_JSON_OPTIONAL(j, options, identityAgent);
        TO_JSON_OPTIONAL(j, options, usePublicKeyAutoAuth);
    }
    void from_json(nlohmann::json const& j, SshOptions& options)
    {
        FROM_JSON_OPTIONAL(j, options, bindAddr);
        FROM_JSON_OPTIONAL(j, options, sshKey);
        FROM_JSON_OPTIONAL(j, options, sshDirectory);
        FROM_JSON_OPTIONAL(j, options, tryAgentForAuthentication);
        FROM_JSON_OPTIONAL(j, options, knownHostsFile);
        FROM_JSON_OPTIONAL(j, options, logVerbosity);
        FROM_JSON_OPTIONAL(j, options, connectTimeoutSeconds);
        FROM_JSON_OPTIONAL(j, options, connectTimeoutUSeconds);
        FROM_JSON_OPTIONAL(j, options, keyExchangeAlgorithms);
        FROM_JSON_OPTIONAL(j, options, compressionClientToServer);
        FROM_JSON_OPTIONAL(j, options, compressionServerToClient);
        FROM_JSON_OPTIONAL(j, options, compressionLevel);
        FROM_JSON_OPTIONAL(j, options, strictHostKeyCheck);
        FROM_JSON_OPTIONAL(j, options, proxyCommand);
        FROM_JSON_OPTIONAL(j, options, gssapiServerIdentity);
        FROM_JSON_OPTIONAL(j, options, gssapiClientIdentity);
        FROM_JSON_OPTIONAL(j, options, gssapiDelegateCredentials);
        FROM_JSON_OPTIONAL(j, options, noDelay);
        FROM_JSON_OPTIONAL(j, options, bypassConfig);
        FROM_JSON_OPTIONAL(j, options, identityAgent);
        FROM_JSON_OPTIONAL(j, options, usePublicKeyAutoAuth);
    }

    void SshOptions::useDefaultsFrom(SshOptions const& other)
    {
        if (!bindAddr.has_value())
            bindAddr = other.bindAddr;
        if (!sshKey.has_value())
            sshKey = other.sshKey;
        if (!sshDirectory.has_value())
            sshDirectory = other.sshDirectory;
        if (!knownHostsFile.has_value())
            knownHostsFile = other.knownHostsFile;
        if (!tryAgentForAuthentication.has_value())
            tryAgentForAuthentication = other.tryAgentForAuthentication;
        if (!logVerbosity.has_value())
            logVerbosity = other.logVerbosity;
        if (!connectTimeoutSeconds.has_value())
            connectTimeoutSeconds = other.connectTimeoutSeconds;
        if (!connectTimeoutUSeconds.has_value())
            connectTimeoutUSeconds = other.connectTimeoutUSeconds;
        if (!keyExchangeAlgorithms.has_value())
            keyExchangeAlgorithms = other.keyExchangeAlgorithms;
        if (!compressionClientToServer.has_value())
            compressionClientToServer = other.compressionClientToServer;
        if (!compressionServerToClient.has_value())
            compressionServerToClient = other.compressionServerToClient;
        if (!compressionLevel.has_value())
            compressionLevel = other.compressionLevel;
        if (!strictHostKeyCheck.has_value())
            strictHostKeyCheck = other.strictHostKeyCheck;
        if (!proxyCommand.has_value())
            proxyCommand = other.proxyCommand;
        if (!gssapiServerIdentity.has_value())
            gssapiServerIdentity = other.gssapiServerIdentity;
        if (!gssapiClientIdentity.has_value())
            gssapiClientIdentity = other.gssapiClientIdentity;
        if (!gssapiDelegateCredentials.has_value())
            gssapiDelegateCredentials = other.gssapiDelegateCredentials;
        if (!noDelay.has_value())
            noDelay = other.noDelay;
        if (!bypassConfig.has_value())
            bypassConfig = other.bypassConfig;
        if (!identityAgent.has_value())
            identityAgent = other.identityAgent;
        if (!usePublicKeyAutoAuth.has_value())
            usePublicKeyAutoAuth = other.usePublicKeyAutoAuth;
    }
}