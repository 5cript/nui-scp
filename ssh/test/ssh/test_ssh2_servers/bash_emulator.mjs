#!/usr/bin/env node

import ssh2 from 'ssh2';
import util from 'node:util';
import blessed from 'blessed';
import fs, { write } from 'node:fs';
import readline from 'node:readline';
import { nanoid } from 'nanoid';
import xtermHeadless from '@xterm/headless';
import minimist from 'minimist';


const { Terminal } = xtermHeadless;

const { Server, utils: { generateKeyPairSync, parseKey } } = ssh2;

const allowedUser = Buffer.from('test');
const allowedPassword = Buffer.from('test');

let logFile;
logFile = {
    write: () => { }
}
// logFile = fs.createWriteStream('./log.txt', { flags: 'w' });

function logMessage(message) {
    logFile.write(`${new Date().toISOString()} - ${message}\n`);
}
logMessage('Server is starting');

function humanFileSize(bytes, si = false, dp = 1) {
    const thresh = si ? 1000 : 1024;

    if (Math.abs(bytes) < thresh) {
        return bytes + ' B';
    }

    const units = si
        ? ['kB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB']
        : ['K', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y'];
    let u = -1;
    const r = 10 ** dp;

    do {
        bytes /= thresh;
        ++u;
    } while (Math.round(Math.abs(bytes) * r) / r >= thresh && u < units.length - 1);


    return bytes.toFixed(dp) + units[u];
}

let server;

const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});

const users = new Map();

const exit = () => {
    users.forEach(user => {
        user.logout();
    });
    server.close();
    rl.close();
    rl.removeAllListeners();
    setTimeout(() => {
        process.exit(0);
    }, 500);
}

/**
 * @param {*} directoryEntryArray { name: string, type: 'file' | 'directory', size: number, date: Date, userName: string, groupName: string, accessMask: number }[]
 */
const makeFakeLs = (directoryEntryArray, options, clientState) => {
    const accessBitsToLsRepresentation = (bits) => {
        switch (bits) {
            case 0b000: return '---';
            case 0b001: return '--x';
            case 0b010: return '-w-';
            case 0b011: return '-wx';
            case 0b100: return 'r--';
            case 0b101: return 'r-x';
            case 0b110: return 'rw-';
            case 0b111: return 'rwx';
        }
        return '???';
    }

    const lsAccessMods = (accessMask) => {
        const user = accessBitsToLsRepresentation((accessMask >> 6) & 0b111);
        const group = accessBitsToLsRepresentation((accessMask >> 3) & 0b111);
        const other = accessBitsToLsRepresentation(accessMask & 0b111);
        return `${user}${group}${other}`;
    }

    if (options.a !== true) {
        directoryEntryArray = directoryEntryArray.filter(entry => entry.name[0] !== '.');
    }

    const maxSizeWidth = Math.max(...directoryEntryArray.map(entry => {
        const size = options.h ? humanFileSize(entry.size) : entry.size.toString();
        return size.length;
    }));

    const colourizeName = (name, type) => {
        if (type === 'directory') {
            return `\x1b[1;34m${name}\x1b[0m`;
        }
        return name;
    }

    const entries = directoryEntryArray.map(entry => {
        const now = new Date();
        const entryDate = entry.date;
        let date;
        if (entryDate.getFullYear() === now.getFullYear()) {
            date = entryDate.toLocaleString('en-US', { month: 'short', day: 'numeric', hour: '2-digit', minute: '2-digit', hour12: false });
        } else {
            date = entryDate.toLocaleString('en-US', { month: 'short', day: 'numeric', year: 'numeric' });
        }

        const dateSplit = date.split(' ')
        date = `${dateSplit[0]} ${dateSplit[1].slice(0, -1).padStart(2, ' ')} ${dateSplit[2].padStart(5, ' ')}`;

        const type = entry.type === 'file' ? '-' : 'd';
        let size = entry.size.toString().padStart(5, ' ');
        let name = entry.name;

        if (options.color) {
            name = colourizeName(name, entry.type);
        }

        if (options.h) {
            size = humanFileSize(entry.size);
        }

        size = size.padStart(maxSizeWidth + 1, ' ');

        if (options.l) {
            return `${type}${lsAccessMods(entry.accessMask)} 1 ${entry.userName} ${entry.groupName} ${size} ${date} ${name}`;
        } else {
            return `${name}`;
        }
    });

    if (options.l) {
        return entries.join('\r\n');
    }
    else {
        const maxWidth = clientState.columns || 80;
        let currentLine = '';
        const lines = [];

        entries.forEach(entry => {
            if ((currentLine + entry + ' ').length > maxWidth) {
                lines.push(currentLine.trim());
                currentLine = '';
            }
            currentLine += entry + ' ';
        });

        if (currentLine) {
            lines.push(currentLine.trim());
        }

        return lines.join('\r\n');
    }
}

class User {
    constructor(stream, client, clientState, id) {
        this.stream = stream;
        this.client = client;
        this.clientState = clientState;
        this.id = id;
    }

    makeScreen() {
        this.screen = new blessed.screen({
            autoPadding: true,
            smartCSR: true,
            program: new blessed.program({
                input: this.stream,
                output: this.stream
            }),
            terminal: this.clientState.terminal || 'ansi',
            cursor: {
                artificial: true,
                shape: 'line',
                blink: true
            }
        });

        this.screen.key(['C-c'], () => {
            this.client.end();
            this.screen.destroy();
        });
        this.screen.key(['C-d'], () => {
            this.client.end();
            this.screen.destroy();
        });

        this.stream.on('resize', () => {
            this.screen.program.emit('resize');
            this.terminal.resize(this.stream.columns, this.stream.rows);
        });

        this.screen.title = 'Hello ' + this.clientState.name;

        this.stream.on('data', (data) => {
            logMessage(`Data received: ${makeSemiHexString(data)}`);
            this.pushData(data);
        });
    }

    setupXterm() {
        this.terminal = new Terminal({
            cols: this.clientState.columns || 80,
            rows: this.clientState.rows || 24,
            allowProposedApi: true
        });

        this.terminal.onData((data) => {
            if (data === '\r') {
                this.stream.write('\r\n');
                this.executeCommand();
                return;
            }

            if (data === '\x7f') {
                this.clientState.commandLineBuffer = this.clientState.commandLineBuffer.slice(0, -1);
                this.stream.write('\b \b');
                return;
            }

            // arrow left:
            if (data === '\x1b[D') {
                this.stream.write('\b');
                return;
            }

            // arrow right:
            if (data === '\x1b[C') {
                this.stream.write('\x1b[C');
                return;
            }

            this.clientState.commandLineBuffer += data;
            this.stream.write(data);
        });
    }

    greet() {
        this.screen.render();
        // XXX This fake resize event is needed for some terminals in order to
        // have everything display correctly
        this.screen.program.emit('resize');

        this.screen.program.clear();
        this.screen.program.write('Welcome to the test server!\r\n');
        this.screen.program.write(this.makePS1());
    }

    logout() {
        this.stream.destroy();
        this.terminal.dispose();
        this.client.end();
        users.delete(this.id);
    }

    disableEcho() {
        this.screen.program.attr('invisible', true);
    }

    makePS1() {
        return `\x1b[1;32m${this.clientState.name} $ \x1b[0m`;
    }

    executeCommand() {
        logMessage(`Command executed: ${makeSemiHexString(this.clientState.commandLineBuffer)}`);
        this.handleCommand(this.clientState.commandLineBuffer);
        this.clientState.commandLineBuffer = '';
        this.stream.write('\r\n');
        this.stream.write(this.makePS1());
    }

    resize(rows, cols) {
        this.clientState.rows = rows;
        this.clientState.columns = cols;
        this.stream.rows = rows;
        this.stream.columns = cols;
        this.terminal.resize(cols, rows);
        this.stream.emit('resize');
    }

    handleCommand(commandLine) {
        const parsed = minimist(commandLine.split(' '));
        const command = parsed._[0];

        if (command === 'exit') {
            this.logout();
            exit();
        } else if (command === 'logout') {
            this.logout();
        } else if (command === 'ls') {
            const makeSemiRandomDate = () => {
                const now = new Date();
                return new Date(now.getTime() - Math.floor(Math.random() * 1000000000));
            }

            this.stream.write(makeFakeLs([
                { name: '.', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.clientState.name, groupName: this.clientState.name, accessMask: 0o755 },
                { name: '..', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.clientState.name, groupName: this.clientState.name, accessMask: 0o755 },
                { name: '.bash_history', type: 'file', size: 9500, date: makeSemiRandomDate(), userName: this.clientState.name, groupName: this.clientState.name, accessMask: 0o644 },
                { name: '.bash_logout', type: 'file', size: 21, date: makeSemiRandomDate(), userName: this.clientState.name, groupName: this.clientState.name, accessMask: 0o644 },
                { name: '.bash_profile', type: 'file', size: 1300, date: makeSemiRandomDate(), userName: this.clientState.name, groupName: this.clientState.name, accessMask: 0o644 },
                { name: '.bashrc', type: 'file', size: 6800, date: makeSemiRandomDate(), userName: this.clientState.name, groupName: this.clientState.name, accessMask: 0o644 },
                { name: '.cache', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.clientState.name, groupName: this.clientState.name, accessMask: 0o755 },
                { name: '.nvm', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.clientState.name, groupName: this.clientState.name, accessMask: 0o755 },
                { name: '.profile', type: 'file', size: 1600, date: makeSemiRandomDate(), userName: this.clientState.name, groupName: this.clientState.name, accessMask: 0o644 },
                { name: 'file1.txt', type: 'file', size: 1234, date: makeSemiRandomDate(), userName: this.clientState.name, groupName: this.clientState.name, accessMask: 0o644 },
                { name: 'file2.log', type: 'file', size: 5678, date: makeSemiRandomDate(), userName: this.clientState.name, groupName: this.clientState.name, accessMask: 0o644 },
                { name: 'Download', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.clientState.name, groupName: this.clientState.name, accessMask: 0o755 },
                { name: 'Documents', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.clientState.name, groupName: this.clientState.name, accessMask: 0o755 },
                { name: 'Pictures', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.clientState.name, groupName: this.clientState.name, accessMask: 0o755 },
                { name: 'Music', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.clientState.name, groupName: this.clientState.name, accessMask: 0o755 },
                { name: 'Videos', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.clientState.name, groupName: this.clientState.name, accessMask: 0o755 },
                { name: 'Desktop', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.clientState.name, groupName: this.clientState.name, accessMask: 0o755 },
            ], parsed, this.clientState));
        }
    }

    pushData(data) {
        const dataString = data.toString();
        this.terminal.input(dataString);
        //this.terminal.write(dataString);
    }
}

const makeSemiHexString = (data) => {
    const hexData = data.toString().split('').map(char => {
        const code = char.charCodeAt(0);
        if (code < 0x20) {
            const hex = code.toString(16).toUpperCase();
            return `\\x${hex.padStart(2, '0')}`;
        }
        return char;
    }).join('');
    return hexData;
};

const shellEmulator = (stream, client, clientState) => {
    const id = nanoid();
    users.set(id, new User(stream, client, clientState, id));

    stream.name = clientState.name;
    stream.rows = clientState.rows || 24;
    stream.columns = clientState.columns || 80;
    stream.isTTY = true;
    stream.setRawMode = (params) => {
        logMessage(`setRawMode (does nothing): ${util.inspect(params)}`);
    };
    stream.on('error', (err) => {
        logMessage(`Stream error: ${util.inspect(err)}`);
    });

    users.get(id).makeScreen();
    users.get(id).setupXterm();
    users.get(id).greet();
    return id;
}


const port = process.argv[2] ? parseInt(process.argv[2], 10) : 0;

server = new Server({
    hostKeys: [fs.readFileSync('./key.private')],
    debug: (message) => {
        logMessage(`Debug: ${message}`);
    }
}, (client) => {
    let clientState = {
        stream: undefined,
        name: 'unknown',
        rows: 24,
        columns: 80,
        terminal: 'ansi',
        commandLineBuffer: ''
    };
    let id = undefined;

    logMessage(`Client connected: ${client.ip}`);
    client.on('authentication', (ctx) => {
        clientState.name = ctx.username;

        if (ctx.method === 'password' && ctx.username === allowedUser.toString() && ctx.password === allowedPassword.toString()) {
            logMessage(`User ${ctx.username} authenticated`);
            ctx.accept();
        } else {
            logMessage(`User ${ctx.username} not authenticated`);
            ctx.reject();
        }
    });
    client.on('ready', () => {
        client.once('session', (accept, reject) => {
            const session = accept();
            session.on('window-change', (accept, reject, info) => {
                if (id) {
                    let user = users.get(id);
                    user.resize(info.rows, info.cols);
                }
                accept && accept();
            });
            session.on('pty', (accept, reject, info) => {
                clientState.rows = info.rows;
                clientState.columns = info.cols;
                clientState.terminal = info.term;
                logMessage(`PTY requested: ${util.inspect(info)}`);
                accept && accept();
            });
            session.once('shell', (accept, reject) => {
                logMessage('Shell requested');
                const stream = accept();
                id = shellEmulator(stream, client, clientState);
            });
            session.on('exec', (accept, reject, info) => {
                logMessage(`Command: ${info.command}`);
                reject();
                // TODO:

                // if (info.command === 'noop') {
                //     const stream = accept();
                //     stream.stderr.write('noop\n');
                //     stream.exit(0);
                //     stream.end();
                //     server.close();
                // } else {
                //     reject();
                // }
            });
        });
    });
});

server.listen(port, '::', () => {
    logMessage(`Listening on port ${server.address().port}`);
    console.log(JSON.stringify({ port: server.address().port }));
});
server.on('error', (err) => {
    logMessage(`Error: ${util.inspect(err)}`);
    console.error(err);
});
server.on('ready', () => {
    logMessage('Server is ready');
});
server.on('close', () => {
    logMessage('Server is closing');
});

rl.on('line', (input) => {
    logMessage(`Command from stdin: ${input}`);
    if (input.length === 0)
        return;

    const command = JSON.parse(input);
    if (Object.hasOwnProperty.call(command, 'command')) {
        if (command.command === 'exit') {
            exit();
        }
        else if (command.command === 'logout') {
            users.get(command.id).logout();
        }
        else if (command.command === 'list') {
            console.log(JSON.stringify(Array.from(users.keys())));
        }
    }
});