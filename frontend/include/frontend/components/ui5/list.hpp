#pragma once

#include <nui/frontend/dom/element.hpp>

// clang-format off

#ifdef NUI_INLINE
// @inline(js, ui5-list)
js_import "@ui5/webcomponents/dist/List.js";
js_import "@ui5/webcomponents/dist/ListItemStandard.js";
js_import "@ui5/webcomponents/dist/ListItemCustom.js";
js_import "@ui5/webcomponents/dist/ListItemGroup.js";
// @endinline
#endif

// clang-format on

namespace ui5
{
    NUI_MAKE_HTML_ELEMENT_RENAME(list, "ui5-list")
    NUI_MAKE_HTML_ELEMENT_RENAME(li, "ui5-li")
    NUI_MAKE_HTML_ELEMENT_RENAME(li_custom, "ui5-li-custom")
    NUI_MAKE_HTML_ELEMENT_RENAME(li_group, "ui5-li-group")
}
