import { Message } from '@lumino/messaging';
import {
    Widget
} from '@lumino/widgets';

class SessionOptions extends Widget {
    static menuFocus: SessionOptions | null;
    deleter: () => any | undefined;

    constructor(name: string, factory: () => HTMLElement, deleter: () => any) {
        super({ node: factory() });
        this.deleter = deleter;

        this.setFlag(Widget.Flag.DisallowLayout);
        this.title.label = name;
        this.title.closable = false;
        this.title.caption = `Pending File Operation Queue`;
    }

    protected onActivateRequest(msg: Message): void {
        if (this.isAttached) {
            // this.inputNode.focus();
        }
    }

    protected onBeforeDetach(msg: Message): void {
        if (SessionOptions.menuFocus === this) {
            SessionOptions.menuFocus = null;
        }
    }

    protected onAfterDetach(msg: Message): void {
        this.deleter();
    }
}

export { SessionOptions };
