#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <utility>

#define TO_JSON_OPTIONAL_RENAME(JSON, CLASS, MEMBER, JSON_NAME) \
    [&](){ \
        if consteval (isStateWrap<decltype(CLASS.MEMBER)>)to_json(JSON[JSON_NAME], *unwrap(CLASS.MEMBER)); \
        else if (CLASS.MEMBER) \
            to_json(JSON[JSON_NAME], *CLASS.MEMBER); \
    }()

#define TO_JSON_OPTIONAL(JSON, CLASS, MEMBER) TO_JSON_OPTIONAL_RENAME(JSON, CLASS, MEMBER, #MEMBER)

#define FROM_JSON_OPTIONAL_RENAME(JSON, CLASS, MEMBER, JSON_NAME) \
    [&](){ \
        if consteval (isStateWrap<decltype(CLASS.MEMBER)>) \
        { \
            const auto proxy = CLASS.MEMBER.modify(); \
            if (auto it = JSON.find(JSON_NAME); it != JSON.end()) \
                unwrap(CLASS.MEMBER) = it->get<std::decay_t<decltype(unwrap(CLASS.MEMBER))>::value_type>(); \
        } \
        else if (auto it = JSON.find(JSON_NAME); it != JSON.end()) \
            CLASS.MEMBER = it->get<std::decay_t<decltype(CLASS.MEMBER)>::value_type>(); \
    }()

#define FROM_JSON_OPTIONAL(JSON, CLASS, MEMBER) FROM_JSON_OPTIONAL_RENAME(JSON, CLASS, MEMBER, #MEMBER)