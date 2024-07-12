#pragma once

#include <persistence/state_core.hpp>
#include <utility/traits_and_concepts/unique_ptr.hpp>

#include <string>
#include <optional>

namespace Persistence
{
    struct Reference
    {
        std::string ref;
        operator std::string() const
        {
            return ref;
        }
    };
    void to_json(nlohmann::json& obj, Reference const& ref);
    void from_json(nlohmann::json const& obj, Reference& ref);

    template <typename T>
    concept ReferenceableType = requires(T t) {
        { t.useDefaultsFrom(std::declval<T>()) } -> std::same_as<void>;
    };

    template <typename T>
    requires ReferenceableType<T>
    class ReferenceAndImpl
    {
      public:
        T const& value() const
        {
            return value_;
        }
        T& value()
        {
            return value_;
        }
        std::string ref() const
        {
            return ref_->ref;
        }
        void ref(std::string const& ref)
        {
            ref_ = Reference{.ref = ref};
        }
        bool hasReference() const
        {
            return ref_.has_value();
        }
        void useDefaultsFrom(T const& other)
        {
            value_.useDefaultsFrom(other);
        }

        ReferenceAndImpl() = default;
        ReferenceAndImpl(Reference ref)
            : ref_{std::move(ref)}
            , value_{}
        {}
        ReferenceAndImpl(T t)
            : ref_{std::nullopt}
            , value_{std::move(t)}
        {}
        ReferenceAndImpl(ReferenceAndImpl const& other)
            : ref_{other.ref_}
            , value_{other.value_}
        {}
        ReferenceAndImpl(ReferenceAndImpl&& other)
            : ref_{std::move(other.ref_)}
            , value_{std::move(other.value_)}
        {}
        ReferenceAndImpl& operator=(ReferenceAndImpl const& other)
        {
            ref_ = other.ref_;
            value_ = other.value_;
            return *this;
        }
        ReferenceAndImpl& operator=(ReferenceAndImpl&& other)
        {
            ref_ = std::move(other.ref_);
            value_ = std::move(other.value_);
            return *this;
        }
        ReferenceAndImpl& operator=(Reference ref)
        {
            ref_ = std::move(ref);
            value_ = std::nullopt;
            return *this;
        }
        ReferenceAndImpl& operator=(T t)
        {
            ref_ = std::nullopt;
            value_ = std::move(t);
            return *this;
        }
        virtual ~ReferenceAndImpl() = default;

      protected:
        std::optional<Reference> ref_;
        T value_;
    };

    template <typename T>
    requires ReferenceableType<T>
    class ReferenceAndImpl<std::unique_ptr<T>>
    {
      public:
        T const& value() const
        {
            return *value_;
        }
        T& value()
        {
            return *value_;
        }
        std::string ref() const
        {
            return ref_->ref;
        }
        void ref(std::string const& ref)
        {
            ref_ = Reference{.ref = ref};
        }
        bool hasReference() const
        {
            return ref_.has_value();
        }
        void useDefaultsFrom(T const& other)
        {
            value_->useDefaultsFrom(other);
        }
        void useDefaultsFrom(std::unique_ptr<T> const& other)
        {
            value_->useDefaultsFrom(*other);
        }

        ReferenceAndImpl() = default;
        ReferenceAndImpl(Reference ref)
            : ref_{std::move(ref)}
            , value_{std::make_unique<T>()}
        {}
        ReferenceAndImpl(T t)
            : ref_{std::nullopt}
            , value_{std::make_unique<T>(std::move(t))}
        {}
        ReferenceAndImpl(ReferenceAndImpl const& other)
            : ref_{other.ref_}
            , value_{[&other]() -> decltype(value_) {
                return std::make_unique<T>(*other.value_);
            }()}
        {}
        ReferenceAndImpl(ReferenceAndImpl&& other)
            : ref_{std::move(other.ref_)}
            , value_{std::move(other.value_)}
        {}
        ReferenceAndImpl& operator=(ReferenceAndImpl const& other)
        {
            ref_ = other.ref_;
            value_ = std::make_unique<T>(*other.value_);
            return *this;
        }
        ReferenceAndImpl& operator=(ReferenceAndImpl&& other)
        {
            ref_ = std::move(other.ref_);
            value_ = std::move(other.value_);
            return *this;
        }
        ReferenceAndImpl& operator=(Reference ref)
        {
            ref_ = std::move(ref);
            value_ = std::make_unique<T>();
            return *this;
        }
        ReferenceAndImpl& operator=(T t)
        {
            ref_ = std::nullopt;
            value_ = std::make_unique<T>(std::move(t));
            return *this;
        }
        ReferenceAndImpl& operator=(std::unique_ptr<T> t)
        {
            ref_ = std::nullopt;
            value_ = std::move(t);
            return *this;
        }
        virtual ~ReferenceAndImpl() = default;

      protected:
        std::optional<Reference> ref_;
        std::unique_ptr<T> value_;
    };

    template <typename T>
    using Referenceable = ReferenceAndImpl<T>;

    template <typename T>
    void to_json(nlohmann::json& obj, Referenceable<T> const& ref)
    {
        if (ref.hasReference())
            obj["$ref"] = ref.ref();

        to_json(obj, ref.value());
    }
    template <Utility::UniquePtrType T>
    void from_json(nlohmann::json const& obj, Referenceable<T>& ref)
    {
        if (obj.contains("$ref"))
        {
            ref.ref(obj["$ref"].get<std::string>());
        }

        from_json(obj, ref.value());
    }
    template <typename T>
    requires(!Utility::IsUniquePtr_v<T>)
    void from_json(nlohmann::json const& obj, Referenceable<T>& ref)
    {
        if (obj.contains("$ref"))
        {
            ref.ref(obj["$ref"].get<std::string>());
        }

        from_json(obj, ref.value());
    }
}