import { Message } from '@lumino/messaging';
import {
    Widget
} from '@lumino/widgets';

class OperationQueue extends Widget {
    static menuFocus: OperationQueue | null;
    deleter: () => any | undefined;

    constructor(name: string, factory: () => HTMLElement, deleter: () => any) {
        super({ node: factory() });
        this.deleter = deleter;

        this.setFlag(Widget.Flag.DisallowLayout);
        this.title.label = name;
        this.title.closable = true;
        this.title.caption = `Pending File Operation Queue`;
    }

    protected onActivateRequest(msg: Message): void {
        if (this.isAttached) {
            // this.inputNode.focus();
        }
    }

    protected onBeforeDetach(msg: Message): void {
        if (OperationQueue.menuFocus === this) {
            OperationQueue.menuFocus = null;
        }
    }

    protected onAfterDetach(msg: Message): void {
        this.deleter();
    }
}

export { OperationQueue };
