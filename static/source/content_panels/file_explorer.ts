import { Message } from '@lumino/messaging';
import {
    Widget
} from '@lumino/widgets';

class FileExplorer extends Widget {
    static menuFocus: FileExplorer | null;
    deleter: () => any | undefined;

    constructor(name: string, factory: () => HTMLElement, deleter: () => any) {
        super({ node: factory() });
        this.deleter = deleter;

        this.setFlag(Widget.Flag.DisallowLayout);
        this.title.label = name;
        this.title.closable = true;
        this.title.caption = `File Explorer`;
    }

    protected onActivateRequest(msg: Message): void {
        if (this.isAttached) {
            // this.inputNode.focus();
        }
    }

    protected onBeforeDetach(msg: Message): void {
        if (FileExplorer.menuFocus === this) {
            FileExplorer.menuFocus = null;
        }
    }

    protected onAfterDetach(msg: Message): void {
        this.deleter();
    }
}

export { FileExplorer };
