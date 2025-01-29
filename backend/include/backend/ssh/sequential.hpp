#pragma once

namespace Detail
{
    struct SequenceResult
    {
        int result{0};
        int index{0};
        int total{0};

        bool reachedEnd() const
        {
            return index == total;
        }
        bool success() const
        {
            return result == 0;
        }
    };

    template <typename... T>
    SequenceResult sequential(T&&... functions)
    {
        int result = 0;
        int index = 0;

        const auto single = [&](auto&& fn) {
            ++index;
            result = fn();
            return result;
        };

        [[maybe_unused]] const bool allRan = ((single(functions) == 0) && ...);

        return {result, index, sizeof...(functions)};
    }
}