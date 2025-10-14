#pragma once

#include <shared_data/shared_data.hpp>
#include <utility/describe.hpp>

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

namespace SharedData
{
    namespace Detail
    {
        struct Empty
        {};
        BOOST_DESCRIBE_STRUCT(Empty, (), ())
    }

    template <typename Data = Detail::Empty>
    struct ErrorOrSuccess : public Data
    {
        std::optional<std::string> error{std::nullopt};
        ErrorOrSuccess() = default;
        ErrorOrSuccess(std::string const& error)
            : Data{}
            , error{error}
        {}
        ErrorOrSuccess(Data data)
            : Data{std::move(data)}
        {}
        bool success() const
        {
            return !error.has_value();
        }
        explicit operator bool() const
        {
            return success();
        }
    };

    template <typename Data = Detail::Empty>
    inline ErrorOrSuccess<Data> success()
    {
        return ErrorOrSuccess<Data>{};
    }
    template <typename Data = Detail::Empty>
    inline ErrorOrSuccess<Data> error(std::string msg)
    {
        return ErrorOrSuccess<Data>{std::move(msg)};
    }

    template <typename Data = Detail::Empty>
    void to_json(nlohmann::json& j, ErrorOrSuccess<Data> const& o)
    {
        if (o.error.has_value())
            j["error"] = o.error.value();
        else
        {
            j = nlohmann::json::object();
            to_json(j, static_cast<Data const&>(o));
            j["success"] = true;
        }
    }
    template <typename Data = Detail::Empty>
    void from_json(nlohmann::json const& j, ErrorOrSuccess<Data>& o)
    {
        if (j.contains("error"))
            o.error = j.at("error").get<std::string>();
        else
        {
            from_json(j, static_cast<Data&>(o));
            o.error = std::nullopt;
        }
    }

#ifdef __EMSCRIPTEN__
    template <typename Data = Detail::Empty>
    inline void to_val(Nui::val& v, ErrorOrSuccess<Data> const& o)
    {
        if (o.error.has_value())
            v = Nui::val::object().set("error", Nui::val::u8string(o.error->c_str()));
        else
        {
            v = Nui::val::object();
            Nui::convertToVal(v, static_cast<Data const&>(o));
            v.set("success", true);
        }
    }
    template <typename Data = Detail::Empty>
    inline void from_val(Nui::val const& v, ErrorOrSuccess<Data>& o)
    {
        if (v.hasOwnProperty("error"))
            o.error = v["error"].template as<std::string>();
        else
        {
            Nui::convertFromVal(v, static_cast<Data&>(o));
            o.error = std::nullopt;
        }
    }
#endif
}