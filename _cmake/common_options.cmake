add_library(core-target INTERFACE)

if (${MSVC})
    target_compile_options(core-target INTERFACE -Wmost)
else()
    target_compile_options(core-target INTERFACE -Wall -Wextra -Wpedantic)
endif()
target_compile_options(core-target INTERFACE -Wbad-function-cast -Wcast-function-type -fexceptions -pedantic $<$<CONFIG:DEBUG>:-g;-Werror=return-type> $<$<CONFIG:RELEASE>:-O3>)
target_link_options(core-target INTERFACE $<$<CONFIG:RELEASE>:-s;-static-libgcc;-static-libstdc++>)
target_compile_features(core-target INTERFACE cxx_std_20)