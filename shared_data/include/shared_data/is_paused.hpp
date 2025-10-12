#pragma once

#include <shared_data/shared_data.hpp>

namespace SharedData
{
    struct IsPaused
    {
        bool paused;
    };
    BOOST_DESCRIBE_STRUCT(IsPaused, (), (paused))
}