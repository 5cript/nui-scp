#pragma once

#if __cplusplus >= 202302L
#    include <expected>
#else
#    include <tl/expected.hpp>
#endif

namespace Utility
{
#ifdef __cpp_lib_expected
    template <typename T, typename E>
    using Expected = std::expected<T, E>;

    template <typename E>
    using Unexpected = std::unexpected<E>;
#else
    template <typename T, typename E>
    using Expected = tl::expected<T, E>;

    template <typename E>
    using Unexpected = tl::unexpected<E>;
#endif
}
