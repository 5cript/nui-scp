import {
    Widget
} from '@lumino/widgets';

class FileExplorer extends Widget {
    static menuFocus: FileExplorer | null;

    static createNode(): HTMLElement {
        let node = document.createElement('div');
        return node;
    }

    constructor(name: string) {
        super({ node: FileExplorer.createNode() });
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
        if (FileExplorer.menuFocus === this) {
            FileExplorer.menuFocus = null;
        }
    }
}

export { FileExplorer };
