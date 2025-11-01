#pragma once

#include <nui/frontend/dom/element.hpp>

// clang-format off

#ifdef NUI_INLINE
// @inline(js, ui5-text)
js_import "@ui5/webcomponents/dist/Text";
// @endinline
#endif

// clang-format on

namespace ui5
{
    NUI_MAKE_HTML_ELEMENT_RENAME(text, "ui5-text")
}