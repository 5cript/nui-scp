#include <persistence/state/queue_options.hpp>

namespace Persistence
{
    void QueueOptions::useDefaultsFrom(QueueOptions const& other)
    {
        if (!autoRemoveCompletedOperations.has_value())
            autoRemoveCompletedOperations = other.autoRemoveCompletedOperations;
        if (!startInPausedState.has_value())
            startInPausedState = other.startInPausedState;
    }
    void to_json(nlohmann::json& j, QueueOptions const& options)
    {
        if (options.autoRemoveCompletedOperations.has_value())
            j["autoRemoveCompletedOperations"] = options.autoRemoveCompletedOperations.value();
        if (options.startInPausedState.has_value())
            j["startInPausedState"] = options.startInPausedState.value();
    }
    void from_json(nlohmann::json const& j, QueueOptions& options)
    {
        if (j.contains("autoRemoveCompletedOperations"))
            options.autoRemoveCompletedOperations = j["autoRemoveCompletedOperations"].get<bool>();
        if (j.contains("startInPausedState"))
            options.startInPausedState = j["startInPausedState"].get<bool>();
    }
}