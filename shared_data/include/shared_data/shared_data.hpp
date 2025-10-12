#pragma once

#include <nui/core.hpp>
#include <nlohmann/json.hpp>
#include <utility/describe.hpp>
#include <utility/traits_and_concepts/optional.hpp>
#include <utility/enum_string_convert.hpp>

#ifdef NUI_FRONTEND
#    include <nui/frontend/val.hpp>
#    include <nui/frontend/utility/val_conversion.hpp>
#endif

namespace SharedData
{
    template <typename EnumT, typename EnumDescription = boost::describe::describe_enumerators<EnumT>>
    void to_json(nlohmann::json& j, EnumT const& e)
    {
        j = Utility::enumToString<EnumT>(e);
    }
    template <typename EnumT, typename EnumDescription = boost::describe::describe_enumerators<EnumT>>
    void from_json(nlohmann::json const& j, EnumT& e)
    {
        e = Utility::enumFromString<EnumT>(j.template get<std::string>());
    }

#ifdef NUI_FRONTEND
    template <typename EnumT, typename EnumDescription = boost::describe::describe_enumerators<EnumT>>
    void to_val(Nui::val& v, EnumT const& e)
    {
        v = Nui::val::u8string(Utility::enumToString<EnumT>(e).c_str());
    }
    template <typename EnumT, typename EnumDescription = boost::describe::describe_enumerators<EnumT>>
    void from_val(Nui::val const& v, EnumT& e)
    {
        e = Utility::enumFromString<EnumT>(v.template as<std::string>());
    }
#endif

    template <
        typename T,
        class Bases = boost::describe::describe_bases<T, boost::describe::mod_any_access>,
        class Members = boost::describe::describe_members<T, boost::describe::mod_any_access>,
        class Enable = std::enable_if_t<!std::is_union_v<T>>>
    void to_json(nlohmann::json& j, T const& obj)
    {
        j = nlohmann::json::object();

        boost::mp11::mp_for_each<Bases>([&](auto&& base) {
            using type = typename std::decay_t<decltype(base)>::type;
            to_json(j, static_cast<type const&>(obj));
        });
        boost::mp11::mp_for_each<Members>([&](auto&& memAccessor) {
            using memberType = std::decay_t<decltype(obj.*memAccessor.pointer)>;
            if constexpr (Utility::OptionalType<memberType>)
            {
                if (obj.*memAccessor.pointer)
                    j[memAccessor.name] = *(obj.*memAccessor.pointer);
            }
            else
            {
                j[memAccessor.name] = obj.*memAccessor.pointer;
            }
        });
    }

    template <
        typename T,
        class Bases = boost::describe::describe_bases<T, boost::describe::mod_any_access>,
        class Members = boost::describe::describe_members<T, boost::describe::mod_any_access>,
        class Enable = std::enable_if_t<!std::is_union_v<T>>>
    void from_json(nlohmann::json const& j, T& obj)
    {
        boost::mp11::mp_for_each<Bases>([&](auto&& base) {
            using type = typename std::decay_t<decltype(base)>::type;
            from_json(j, static_cast<type&>(obj));
        });
        boost::mp11::mp_for_each<Members>([&](auto&& memAccessor) {
            using memberType = std::decay_t<decltype(obj.*memAccessor.pointer)>;
            if constexpr (Utility::OptionalType<memberType>)
            {
                if (j.contains(memAccessor.name))
                    j.at(memAccessor.name).get_to(obj.*memAccessor.pointer);
                else
                    obj.*memAccessor.pointer = std::nullopt;
            }
            else
            {
                if (j.contains(memAccessor.name))
                    j.at(memAccessor.name).get_to(obj.*memAccessor.pointer);
                else
                    throw std::runtime_error(
                        std::string("Missing required field '") + memAccessor.name + "' in JSON object");
            }
        });
    }
}