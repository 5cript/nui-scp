#include <persistence/state_holder.hpp>

namespace Persistence
{
    struct StateHolder::Implementation
    {
        State state;
    };

    StateHolder::StateHolder()
        : impl_(std::make_unique<Implementation>())
    {}
    ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(StateHolder);
} // namespace Persistence