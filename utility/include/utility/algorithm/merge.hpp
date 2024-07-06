#pragma once

namespace Utility::Algorithm
{
    template <typename T, template <typename...> typename ContainerT>
    ContainerT<T> merge(ContainerT<T> const& lhs, ContainerT<T> const& rhs)
    {
        ContainerT<T> result;
        result.reserve(lhs.size() + rhs.size());
        std::merge(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), std::back_inserter(result));
        return result;
    }

    template <typename T, template <typename...> typename ContainerT>
    ContainerT<T> mergeIntoFirst(ContainerT<T>& lhs, ContainerT<T> const& rhs)
    {
        lhs.reserve(lhs.size() + rhs.size());
        std::merge(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), std::back_inserter(lhs));
        return lhs;
    }
}