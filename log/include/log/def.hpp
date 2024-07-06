#ifdef __EMSCRIPTEN__
#    include <fmt/format.h>
#else
#    include <spdlog/spdlog.h>
#endif

#ifdef __EMSCRIPTEN__
template <typename... Args>
using format_string = fmt::format_string<Args...>;
#else
template <typename... Args>
using format_string = spdlog::format_string_t<Args...>;
#endif