// import './livereload.ts';
import { nanoid } from "nanoid";

import "@ui5/webcomponents/dist/Assets.js";
import "@ui5/webcomponents-fiori/dist/Assets.js";
import { setTheme } from "@ui5/webcomponents-base/dist/config/Theme.js";
import { setThemeRoot } from "@ui5/webcomponents-base/dist/config/ThemeRoot.js";

(() => {
    globalThis.nanoid = nanoid;

    setTheme("dark");
    // app.example is intentional, because of windows dns timeouts
    setThemeRoot("nui://app.example/");
})();
