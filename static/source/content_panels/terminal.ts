import {
    Widget
} from '@lumino/widgets';

class Terminal extends Widget {
    static menuFocus: Terminal | null;

    static createNode(): HTMLElement {
        // let node = document.createElement('div');
        // let content = document.createElement('div');
        // node.appendChild(content);
        // return node;
    }

    constructor(name: string, factory: () => HTMLElement) {
        super({ node: factory() });
        this.setFlag(Widget.Flag.DisallowLayout);
        //this.addClass('content');
        this.title.label = name;
        this.title.closable = false;
        this.title.caption = `Long description for: ${name}`;
    }

    protected onActivateRequest(msg: Message): void {
        if (this.isAttached) {
            // focus terminal:
            //this.inputNode.focus();
        }
    }

    protected onBeforeDetach(msg: Message): void {
        if (Terminal.menuFocus === this) {
            Terminal.menuFocus = null;
        }
    }
}

export { Terminal };