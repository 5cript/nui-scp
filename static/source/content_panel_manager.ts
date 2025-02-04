import { ContentPanel } from './content_panel';
import { Terminal } from './content_panels/terminal';
import { FileExplorer } from './content_panels/file_explorer';
import { OperationQueue } from './content_panels/operation_queue';
import { SessionOptions } from './content_panels/session_options';
import {
    BoxPanel,
    DockPanel,
    Widget,
    SplitLayout,
    DockLayout,
} from '@lumino/widgets';

class ContentPanelManager {
    panels: Map<string, ContentPanel>;
    fileExplorerFactory: () => HTMLElement | undefined;
    fileExplorerDelete: () => any | undefined;
    operationQueueFactory: () => HTMLElement | undefined;
    operationQueueDelete: () => any | undefined;
    sessionOptionsFactory: () => HTMLElement | undefined;
    sessionOptionsDelete: () => any | undefined;

    constructor() {
        this.panels = new Map<string, ContentPanel>();
        this.fileExplorerDelete = () => { return undefined; };
        this.fileExplorerFactory = () => { return undefined; };
        this.operationQueueDelete = () => { return undefined; };
        this.operationQueueFactory = () => { return undefined; };
        this.sessionOptionsDelete = () => { return undefined; };
        this.sessionOptionsFactory = () => { return undefined; };
    }

    modifyDefaultLayout(dock: DockPanel) {
        const saved = dock.saveLayout();
        const main = saved.main as DockLayout.ISplitAreaConfig;
        // ((dock.saveLayout().main as DockLayout.ISplitAreaConfig)?.children[0] as DockLayout.ISplitAreaConfig)?.sizes
        if (main) {
            const children = main.children;
            if (children) {
                const first = children[0] as DockLayout.ISplitAreaConfig;
                if (first) {
                    first.sizes = [0.7, 0.3];
                }
            }
        }
        dock.restoreLayout(saved);
    }

    addPanel(
        host: HTMLElement,
        id: string,
        terminalFactory: () => HTMLElement,
        fileExplorerFactory: () => HTMLElement,
        fileExplorerDelete: () => any,
        operationQueueFactory: () => HTMLElement,
        operationQueueDelete: () => any,
        sessionOptionsFactory: () => HTMLElement,
        sessionOptionsDelete: () => any
    ) {
        this.fileExplorerFactory = fileExplorerFactory;
        this.fileExplorerDelete = fileExplorerDelete;
        this.operationQueueFactory = operationQueueFactory;
        this.operationQueueDelete = operationQueueDelete;
        this.sessionOptionsFactory = sessionOptionsFactory;
        this.sessionOptionsDelete = sessionOptionsDelete;

        let term = new Terminal('Terminal', terminalFactory);
        let explorer = new FileExplorer('FileExplorer', fileExplorerFactory, fileExplorerDelete);
        let queue = new OperationQueue('OperationQueue', operationQueueFactory, operationQueueDelete);
        let options = new SessionOptions('SessionOptions', sessionOptionsFactory, sessionOptionsDelete);

        let dock = new DockPanel({
            addButtonEnabled: true,
        });
        dock.addWidget(term);
        dock.addWidget(explorer, { mode: 'split-right', ref: term });
        dock.addWidget(queue, { mode: "split-bottom", ref: term });
        dock.addWidget(options, { ref: queue });
        dock.id = 'dock_' + id;

        this.modifyDefaultLayout(dock);

        // dock.layoutModified.connect(() => {
        //     console.log("layout modified");
        //     const layout = dock.layout;
        //     if (layout) {
        //         console.log(layout);
        //     }
        // });
        dock.addRequested.connect((sender, widget) => {
            console.log("add requested");
        });

        let main = new BoxPanel({ spacing: 0 });
        main.id = 'main_' + id;
        main.addWidget(dock);

        const resizeObserver = new ResizeObserver(() => {
            main.update();
        });
        resizeObserver.observe(host);

        try {
            Widget.attach(main, host);
            main.update();
        }
        catch (e) {
            console.log(e);
            console.log(JSON.stringify(e));
        }
        this.panels.set(id, main);
    }

    getPanelById(id: string): ContentPanel | undefined {
        return this.panels.get(id);
    }

    getDockPanelById(id: string): DockPanel | undefined {
        let panel = this.panels.get(id);
        if (panel) {
            return panel.widgets[0] as DockPanel;
        }
        return undefined;
    }

    getPanelLayout(id: string): any | undefined {
        let dock = this.getDockPanelById(id);
        if (dock) {
            return JSON.parse(JSON.stringify(dock.saveLayout(), (key, value) => {
                if (key === 'widgets') {
                    return value.map((e: any) => e.node.id);
                }
                return value;
            }));
        }
        return undefined;
    }

    loadPanelLayout(id: string, layout: any) {
        // TODO: NOTE!!!!!: Make it possible to save and load multiple layouts and inherit them in the persistence.
        // So you can save a terminal only layout for instance and a file only and a full layout and then load which you want.

        // https://vk.github.io/dash-lumino-components/DockPanel.react.js.html

        // newlayout = JSON.parse(JSON.stringify(newlayout));

        // function updateWidgets(layout, components) {
        //     if (!layout || typeof layout !== 'object') {
        //         return; // Check if layout is null or not an object
        //     }

        //     if (Array.isArray(layout)) {
        //         layout.forEach(item => updateWidgets(item, components));
        //     } else {
        //         Object.keys(layout).forEach(key => {
        //             if (Array.isArray(layout[key])) {
        //                 if (key === "widgets") {

        //                     layout[key] = layout[key].map(widget => (components[widget] && components[widget].lumino) ? components[widget].lumino : null).filter(w => w !== null);
        //                 } else {
        //                     updateWidgets(layout[key], components);
        //                 }
        //             } else if (typeof layout[key] === 'object') {
        //                 updateWidgets(layout[key], components);
        //             }
        //         });
        //     }
        // }

        // updateWidgets(newlayout, components);
        // components[this.props.id].lumino.restoreLayout(newlayout);
    }

    removePanel = (id: string) => {
        console.log("remove panel");
        let main = document.getElementById('main_' + id);
        if (main) {
            main.remove();
        }
        this.panels.delete(id);
    }
};
export { ContentPanelManager };