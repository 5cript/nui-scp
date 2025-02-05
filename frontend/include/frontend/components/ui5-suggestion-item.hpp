#pragma once

#include <nui/frontend/dom/element.hpp>

// clang-format off

#ifdef NUI_INLINE
// @inline(js, ui5-suggestion-item)
js_import "@ui5/webcomponents/dist/SuggestionItem.js";
js_import "@ui5/webcomponents/dist/features/InputSuggestions.js";
// @endinline
#endif

// clang-format on

namespace ui5
{
    NUI_MAKE_HTML_ELEMENT_RENAME(suggestion_item, "ui5-suggestion-item")
}