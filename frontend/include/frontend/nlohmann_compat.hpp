#pragma once

#include <nlohmann/json.hpp>
#include <nui/frontend/api/json.hpp>

template <typename T>
Nui::val asVal(T const& value)
{
    return Nui::JSON::parse(nlohmann::json(value).dump());
}