#pragma once

#include <algorithm>
#include <bit>

namespace Utility
{
    template <std::integral T>
    constexpr T byteswap(T value) noexcept
    {
        static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");
        auto value_representation = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
        std::reverse(value_representation.begin(), value_representation.end());
        return std::bit_cast<T>(value_representation);
    }
}