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
        this.title.caption = `Long description for: ${name}`;
    }

    protected onActivateRequest(msg: Message): void {
        if (this.isAttached) {
            // this.inputNode.focus();
        }
    }

    protected onBeforeDetach(msg: Message): void {
        this.deleter();
        if (FileExplorer.menuFocus === this) {
            FileExplorer.menuFocus = null;
        }
    }
}

export { FileExplorer };
