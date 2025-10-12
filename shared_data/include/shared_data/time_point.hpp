#pragma once

#include <chrono>

#include <shared_data/shared_data.hpp>
#include <fmt/format.h>
#include <fmt/chrono.h>

#include <sstream>

namespace SharedData
{
#ifdef NUI_FRONTEND
    inline void to_val(Nui::val& v, std::chrono::time_point<std::chrono::system_clock> const& tp)
    {
        v = Nui::val::u8string(fmt::format("{:%Y-%m-%d %H:%M:%S}", tp).c_str());
    }
    inline void from_val(Nui::val const& v, std::chrono::time_point<std::chrono::system_clock>& tp)
    {
        std::istringstream in{v.template as<std::string>()};
        std::tm tm = {};
        in >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        if (in.fail())
            throw std::invalid_argument("Failed to parse time_point from string: " + v.template as<std::string>());
        tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }
#endif
    inline void to_json(nlohmann::json& j, std::chrono::time_point<std::chrono::system_clock> const& tp)
    {
        j = fmt::format("{:%Y-%m-%d %H:%M:%S}", tp);
    }
    inline void from_json(nlohmann::json const& j, std::chrono::time_point<std::chrono::system_clock>& tp)
    {
        std::istringstream in{j.get<std::string>()};
        std::tm tm = {};
        in >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        if (in.fail())
            throw std::invalid_argument("Failed to parse time_point from string: " + j.get<std::string>());
        tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }
}

// Inject into STD for ADL:
namespace std::chrono
{
#ifdef NUI_FRONTEND
    inline void to_val(Nui::val& v, time_point<system_clock> const& tp)
    {
        SharedData::to_val(v, tp);
    }
    inline void from_val(Nui::val const& v, time_point<system_clock>& tp)
    {
        SharedData::from_val(v, tp);
    }
#endif
    inline void to_json(nlohmann::json& j, time_point<system_clock> const& tp)
    {
        SharedData::to_json(j, tp);
    }
    inline void from_json(nlohmann::json const& j, time_point<system_clock>& tp)
    {
        SharedData::from_json(j, tp);
    }
}