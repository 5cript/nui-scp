import { Message } from '@lumino/messaging';
import {
    NuiWidget
} from './nui_widget';

class FileExplorer extends NuiWidget {
    constructor(name: string, factory: () => HTMLElement | undefined, deleter: () => any) {
        super(name, factory, deleter, 'file-explorer');

        this.title.label = name;
        this.title.closable = true;
        this.title.caption = `File Explorer`;
    }
}

export { FileExplorer };
