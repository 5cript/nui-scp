// import './livereload.ts';

import "@ui5/webcomponents/dist/Assets.js";
import "@ui5/webcomponents-fiori/dist/Assets.js";
import "@ui5/webcomponents-base/dist/Device.js";
import "@ui5/webcomponents/dist/features/InputSuggestions.js";
import { setTheme } from "@ui5/webcomponents-base/dist/config/Theme.js";
import { setThemeRoot } from "@ui5/webcomponents-base/dist/config/ThemeRoot.js";

(() => {
    globalThis.generateId = () => {
        return crypto.randomUUID();
    }

    setTheme("dark");
    // app.example is intentional, because of windows dns timeouts
    setThemeRoot("nui://app.example/");
})();
