#pragma once

#include <utility/overloaded.hpp>

#include <variant>

#pragma once

namespace Utility
{
    template <typename... VariantTypes, typename... VisitFunctionTypes>
    auto visitOverloaded(std::variant<VariantTypes...> const& variant, VisitFunctionTypes&&... visitFunctions)
    {
        return std::visit(overloaded{std::forward<VisitFunctionTypes>(visitFunctions)...}, variant);
    }
    template <typename... VariantTypes, typename... VisitFunctionTypes>
    auto visitOverloaded(std::variant<VariantTypes...>& variant, VisitFunctionTypes&&... visitFunctions)
    {
        return std::visit(overloaded{std::forward<VisitFunctionTypes>(visitFunctions)...}, variant);
    }
    template <typename... VariantTypes, typename... VisitFunctionTypes>
    auto visitOverloaded(std::variant<VariantTypes...>&& variant, VisitFunctionTypes&&... visitFunctions)
    {
        return std::visit(overloaded{std::forward<VisitFunctionTypes>(visitFunctions)...}, std::move(variant));
    }
}