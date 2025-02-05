import { Message } from '@lumino/messaging';
import {
    NuiWidget
} from './nui_widget';

class Terminal extends NuiWidget {
    constructor(name: string, factory: () => HTMLElement | undefined, deleter: () => any) {
        super(name, factory, deleter, 'terminal');

        this.title.label = 'Terminal';
        this.title.closable = true;
        this.title.caption = 'Terminal';
    }
}

export { Terminal };
