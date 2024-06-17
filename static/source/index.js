// import './livereload.ts';
import { nanoid } from "nanoid";

(() => {
    globalThis.nanoid = nanoid;
})();
