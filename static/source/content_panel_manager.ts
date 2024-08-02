import { ContentPanel } from './content_panel';
import { Terminal } from './content_panels/terminal';
import { FileExplorer } from './content_panels/file_explorer';
import {
    BoxPanel,
    DockPanel,
    Widget
} from '@lumino/widgets';

class ContentPanelManager {
    panels: Map<string, ContentPanel>;

    constructor() {
        this.panels = new Map<string, ContentPanel>();
    }

    addPanel(host: HTMLElement, id: string, terminalFactory: () => HTMLElement) {
        console.log("1");
        let term = new Terminal('Terminal', terminalFactory);
        let explorer = new FileExplorer('FileExplorer');

        console.log("a");
        let dock = new DockPanel();
        dock.addWidget(term);
        dock.addWidget(explorer, { mode: 'split-right', ref: term });
        dock.id = 'dock_' + id;

        console.log("b");
        let main = new BoxPanel({ direction: 'left-to-right', spacing: 0 });
        main.id = 'main_' + id;
        main.addWidget(dock);

        // host.onresize = () => {
        //     console.log("resize");
        //     main.update();
        // };

        console.log("c");
        console.log(host.isConnected);
        try {
            Widget.attach(main, host);
            console.log(host.isConnected);
            main.update();
        }
        catch (e) {
            console.log(e);
            console.log(JSON.stringify(e));
        }
        console.log("d");
        this.panels.set(id, main);
    }

    removePanel = (id: string) => {
        console.log("remove");
        let main = document.getElementById('main_' + id);
        if (main) {
            main.remove();
        }
        this.panels.delete(id);
    }

    X = (host: HTMLElement, id: string, terminalFactory: () => HTMLElement) => {
        terminalFactory();
    }
};
export { ContentPanelManager };