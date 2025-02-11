import { ContentPanel } from './content_panel';
import { Terminal } from './content_panels/terminal';
import { FileExplorer } from './content_panels/file_explorer';
import { OperationQueue } from './content_panels/operation_queue';
import { SessionOptions } from './content_panels/session_options';
import {
    BoxPanel,
    DockPanel,
    Widget,
    TabBar,
    DockLayout
} from '@lumino/widgets';
import {
    ChannelId
} from './ids.ts';

class ContentPanelManager {
    panels: Map<string, ContentPanel>;
    terminalFactory: () => HTMLElement | undefined;
    terminalDelete: (channelId: ChannelId | undefined) => any;
    fileExplorerFactory: () => HTMLElement | undefined;
    fileExplorerDelete: () => any;
    operationQueueFactory: () => HTMLElement | undefined;
    operationQueueDelete: () => any;
    sessionOptionsFactory: () => HTMLElement | undefined;
    sessionOptionsDelete: () => any;

    constructor() {
        this.panels = new Map<string, ContentPanel>();
        this.terminalDelete = (_: ChannelId | undefined) => { return undefined; };
        this.terminalFactory = () => { return undefined; };
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

    private makeDefaultDock(id: string): DockPanel {
        let term = new Terminal('Terminal', this.terminalFactory, this.terminalDelete);
        let explorer = new FileExplorer('FileExplorer', this.fileExplorerFactory, this.fileExplorerDelete);
        let queue = new OperationQueue('OperationQueue', this.operationQueueFactory, this.operationQueueDelete);
        let options = new SessionOptions('SessionOptions', this.sessionOptionsFactory, this.sessionOptionsDelete);

        let dock = new DockPanel({
            addButtonEnabled: true,
        });
        dock.addWidget(term);
        dock.addWidget(explorer, { mode: 'split-right', ref: term });
        dock.addWidget(queue, { mode: "split-bottom", ref: term });
        dock.addWidget(options, { ref: queue });
        dock.id = 'dock_' + id;
        this.modifyDefaultLayout(dock);

        return dock;
    }

    private fabricateComponentFromId(id: string): Widget | undefined {
        switch (id) {
            case 'terminal':
                return new Terminal('Terminal', this.terminalFactory, this.terminalDelete);
            case 'file-explorer':
                return new FileExplorer('FileExplorer', this.fileExplorerFactory, this.fileExplorerDelete);
            case 'operation-queue':
                return new OperationQueue('OperationQueue', this.operationQueueFactory, this.operationQueueDelete);
            case 'session-options':
                return new SessionOptions('SessionOptions', this.sessionOptionsFactory, this.sessionOptionsDelete);
            default:
                return undefined;
        }
    }

    private makeDockFromLayout(id: string, layoutString: string): DockPanel {
        let dehydrated = JSON.parse(layoutString);

        let deserializeArea = (area: any) => {
            if (!area) {
                return null;
            }

            const type = ((area as any).type as string) || 'unknown';
            if (type === 'unknown' || (type !== 'tab-area' && type !== 'split-area')) {
                console.error(`Attempted to deserialize unknown type: ${type}`);
                return null;
            }

            // Currently everything can only be constructed once!
            const usedUpIds = new Set<string>();

            if (type === 'tab-area') {
                const { currentIndex, widgets } = area;

                const hydrated = {
                    type: 'tab-area',
                    currentIndex: currentIndex || 0,
                    widgets:
                        (widgets &&
                            (widgets.map(widget => {
                                if (usedUpIds.has(widget)) {
                                    console.error(`Duplicate widget id: ${widget}`);
                                    return null;
                                }
                                usedUpIds.add(widget);
                                return this.fabricateComponentFromId(widget);
                            }).filter(widget => !!widget))) || [],
                };

                if (hydrated.currentIndex > hydrated.widgets.length - 1) {
                    hydrated.currentIndex = 0;
                }

                return hydrated;
            }

            if (type === 'split-area') {
                const { orientation, sizes, children } = area;

                const hydrated = {
                    type: 'split-area',
                    orientation,
                    sizes: sizes || [],
                    children:
                        (children &&
                            (children.map(child => {
                                return deserializeArea(child);
                            }).filter(child => !!child))) || [],
                };

                return hydrated;
            }
        };

        let dock = new DockPanel({
            addButtonEnabled: true,
        });

        const area = { main: deserializeArea(dehydrated.main) };
        if (area) {
            const dockLayout = dock.layout as DockLayout;
            dockLayout.restoreLayout(area as DockPanel.ILayoutConfig);
        }

        dock.id = 'dock_' + id;

        return dock;
    }

    addPanel(
        host: HTMLElement,
        id: string,
        layoutString: string,
        terminalFactory: () => HTMLElement,
        terminalDelete: (channelId: ChannelId | undefined) => any,
        fileExplorerFactory: () => HTMLElement,
        fileExplorerDelete: () => any,
        operationQueueFactory: () => HTMLElement,
        operationQueueDelete: () => any,
        sessionOptionsFactory: () => HTMLElement,
        sessionOptionsDelete: () => any
    ) {
        this.terminalFactory = terminalFactory;
        this.terminalDelete = terminalDelete;
        this.fileExplorerFactory = fileExplorerFactory;
        this.fileExplorerDelete = fileExplorerDelete;
        this.operationQueueFactory = operationQueueFactory;
        this.operationQueueDelete = operationQueueDelete;
        this.sessionOptionsFactory = sessionOptionsFactory;
        this.sessionOptionsDelete = sessionOptionsDelete;

        let main = new BoxPanel({ spacing: 0 });
        main.id = 'main_' + id;

        let dock: DockPanel;
        if (layoutString === "") {
            dock = this.makeDefaultDock(id);
        } else {
            dock = this.makeDockFromLayout(id, layoutString);
        }

        dock.addRequested.connect((sender: DockPanel, widget: TabBar<Widget>) => {
            console.log("add requested");
            dock.addWidget(this.fabricateComponentFromId('terminal') as Widget, { ref: widget.titles[0].owner });
        });
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
                    return value.map((e: any) => e.layoutId);
                }
                return value;
            }));
        }
        return undefined;
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