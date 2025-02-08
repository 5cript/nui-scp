#pragma once

#include <string>

#ifdef __EMSCRIPTEN__
#    include <nui/frontend/val.hpp>
#else
#    include <boost/uuid/uuid_generators.hpp>
#    include <boost/uuid/uuid.hpp>
#    include <boost/uuid/uuid_io.hpp>
#endif

namespace Ids
{
    class Id
    {
      public:
        friend Id generateId();

        Id(Id const&) = default;
        Id(Id&&) = default;
        Id& operator=(Id const&) = default;
        Id& operator=(Id&&) = default;
        ~Id() = default;

        std::string const& id() const
        {
            return id_;
        }

        auto operator*() const
        {
            return id_;
        }

        std::string value() const
        {
            return id_;
        }

        friend bool operator==(Id const& lhs, Id const& rhs)
        {
            return lhs.id_ == rhs.id_;
        }

        bool isValid() const
        {
            return id_ != "INVALID_ID";
        }

      protected:
        Id() = delete;
        explicit Id(std::string const& id)
            : id_{id}
        {}

      private:
        std::string id_;
    };

    struct IdHash
    {
        template <typename T>
        std::size_t operator()(T const& id) const
        {
            return std::hash<std::string>{}(id.value());
        }
    };

#ifdef __EMSCRIPTEN__
    inline Id generateId()
    {
        return Id{Nui::val::global("generateId")().as<std::string>()};
    }
#else
    inline Id generateId()
    {
        std::stringstream sstr;
        sstr << boost::uuids::random_generator()();
        return Id{sstr.str()};
    }
#endif
}

#define DEFINE_ID_TYPE(name) \
    namespace Ids \
    { \
        class name : public Id \
        { \
          public: \
            friend name generate##name(); \
            friend name make##name(std::string const&); \
\
          public: \
            name() \
                : Id{"INVALID_ID"} \
            {} \
\
          private: \
            name(std::string const& str) \
                : Id{str} \
            {} \
        }; \
\
        inline name generate##name() \
        { \
            return name{generateId().value()}; \
        } \
\
        inline name make##name(std::string const& str) \
        { \
            return name{str}; \
        } \
    }