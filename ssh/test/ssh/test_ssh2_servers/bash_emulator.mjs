#!/usr/bin/env node

import ssh2 from 'ssh2';
import util from 'node:util';
import fs, { write } from 'node:fs';
import readline from 'node:readline';
import { nanoid } from 'nanoid';
import xtermHeadless from '@xterm/headless';
import minimist from 'minimist';
import tkit from 'terminal-kit/lib/termkit-no-lazy-require.js';

const { Terminal } = xtermHeadless;

const { Server, utils: { generateKeyPairSync, parseKey } } = ssh2;

const allowedUser = Buffer.from('test');
const allowedPassword = Buffer.from('test');

const programArgs = minimist(process.argv.slice(2));

const port = programArgs.port || 0;
const verbose = programArgs.verbose || false;

let logFile;
// logFile = fs.createWriteStream('./log.txt', { flags: 'w' });
logFile = {
    write: (msg) => {
    }
}

function logMessage(message) {
    // logFile.write(`${new Date().toISOString()} - ${message}\n`);
    if (verbose) {
        console.log(message);
    }
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
        user.dispose();
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
const makeFakeLs = (directoryEntryArray, options, channelState) => {
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
        return entries.join('\r\n') + '\r\n';
    }
    else {
        const maxWidth = channelState.columns || 80;
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

        return lines.join('\r\n') + '\r\n';
    }
}

class Channel {
    constructor(stream, client, channelState, channelId) {
        this.stream = stream;
        this.client = client;
        this.channelState = channelState;
        this.channelId = channelId;
    }

    makeScreen() {
        this.screen = tkit.createTerminal({
            stdin: this.stream,
            stdout: this.stream,
        });

        this.screen.width = this.channelState.columns || 80;
        this.screen.height = this.channelState.rows || 24;

        this.stream.on('resize', () => {
            this.screen.width = this.stream.columns;
            this.screen.height = this.stream.rows;
        });

        this.screen.grabInput();

        this.screen.on('key', (name, matches, data) => {
            if (name === 'CTRL_C')
                this.logout();
            else if (name === 'CTRL_D')
                this.logout();
        });

        this.stream.on('data', (data) => {
            logMessage(`${this.channelId}: Data received: ${makeSemiHexString(data)}`);
            this.pushData(data);
        });

        this.clear();
    }

    setupXterm() {
        this.terminal = new Terminal({
            cols: this.channelState.columns || 80,
            rows: this.channelState.rows || 24,
            allowProposedApi: true
        });

        this.terminal.onData((data) => {
            if (data === '\r') {
                this.stream.write('\r\n');
                this.executeCommand();
                return;
            }

            if (data === '\x7f') {
                this.channelState.commandLineBuffer = this.channelState.commandLineBuffer.slice(0, -1);
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

            this.channelState.commandLineBuffer += data;
            this.stream.write(data);
        });
    }

    greet() {
        this.screen.clear();
        this.stream.write('Welcome to the test server!\r\n');
        this.stream.write(this.makePS1());
    }

    clear() {
        this.screen.clear();
        this.screen.moveTo(1, 1);
    }

    logout() {
        this.client.end();
        //this.screen.destroy();
        this.terminal.dispose();
    }

    disableEcho() {
        this.stream.write('\x1b[?25l');
    }

    makePS1() {
        return `\x1b[1;32m${this.channelState.name} $ \x1b[0m`;
    }

    executeCommand() {
        logMessage(`${this.channelId}: Command executed: ${makeSemiHexString(this.channelState.commandLineBuffer)}`);
        this.handleCommand(this.channelState.commandLineBuffer);
        this.channelState.commandLineBuffer = '';
        this.stream.write(this.makePS1());
    }

    resize(rows, cols) {
        this.channelState.rows = rows;
        this.channelState.columns = cols;
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
        } else if (command === 'echo') {
            this.stream.write(parsed._.slice(1).join(' '));
        } else if (command === 'pwd') {
            this.stream.write('/home/' + this.channelState.name);
        } else if (command === 'clear') {
            this.clear();
        } else if (command === 'ls') {
            const makeSemiRandomDate = () => {
                const now = new Date();
                return new Date(now.getTime() - Math.floor(Math.random() * 1000000000));
            }

            this.stream.write(makeFakeLs([
                { name: '.', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.channelState.name, groupName: this.channelState.name, accessMask: 0o755 },
                { name: '..', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.channelState.name, groupName: this.channelState.name, accessMask: 0o755 },
                { name: '.bash_history', type: 'file', size: 9500, date: makeSemiRandomDate(), userName: this.channelState.name, groupName: this.channelState.name, accessMask: 0o644 },
                { name: '.bash_logout', type: 'file', size: 21, date: makeSemiRandomDate(), userName: this.channelState.name, groupName: this.channelState.name, accessMask: 0o644 },
                { name: '.bash_profile', type: 'file', size: 1300, date: makeSemiRandomDate(), userName: this.channelState.name, groupName: this.channelState.name, accessMask: 0o644 },
                { name: '.bashrc', type: 'file', size: 6800, date: makeSemiRandomDate(), userName: this.channelState.name, groupName: this.channelState.name, accessMask: 0o644 },
                { name: '.cache', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.channelState.name, groupName: this.channelState.name, accessMask: 0o755 },
                { name: '.nvm', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.channelState.name, groupName: this.channelState.name, accessMask: 0o755 },
                { name: '.profile', type: 'file', size: 1600, date: makeSemiRandomDate(), userName: this.channelState.name, groupName: this.channelState.name, accessMask: 0o644 },
                { name: 'file1.txt', type: 'file', size: 1234, date: makeSemiRandomDate(), userName: this.channelState.name, groupName: this.channelState.name, accessMask: 0o644 },
                { name: 'file2.log', type: 'file', size: 5678, date: makeSemiRandomDate(), userName: this.channelState.name, groupName: this.channelState.name, accessMask: 0o644 },
                { name: 'Download', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.channelState.name, groupName: this.channelState.name, accessMask: 0o755 },
                { name: 'Documents', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.channelState.name, groupName: this.channelState.name, accessMask: 0o755 },
                { name: 'Pictures', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.channelState.name, groupName: this.channelState.name, accessMask: 0o755 },
                { name: 'Music', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.channelState.name, groupName: this.channelState.name, accessMask: 0o755 },
                { name: 'Videos', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.channelState.name, groupName: this.channelState.name, accessMask: 0o755 },
                { name: 'Desktop', type: 'directory', size: 0, date: makeSemiRandomDate(), userName: this.channelState.name, groupName: this.channelState.name, accessMask: 0o755 },
            ], parsed, this.channelState));
        }
    }

    pushData(data) {
        const dataString = data.toString();
        this.terminal.input(dataString);
    }
}

class Client {
    constructor(id, client) {
        this.id = id;
        this.client = client;
        this.name = 'unknown';
        this.channels = new Map();
    }

    addChannel(channelId, stream, channelState) {
        this.channels.set(channelId, new Channel(stream, this.client, channelState, channelId));
    }

    removeChannel(channelId) {
        this.channels.get(channelId).logout();
        this.channels.delete(channelId);
    }

    getChannel(channelId) {
        return this.channels.get(channelId);
    }

    dispose() {
        this.channels.forEach(channel => {
            channel.logout();
        });

        users.delete(this.id);
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

const shellEmulator = (clientId, channelId, stream, client, channelState) => {
    users.get(clientId).addChannel(channelId, stream, channelState);

    stream.name = channelState.name;
    stream.rows = channelState.rows || 24;
    stream.columns = channelState.columns || 80;
    stream.isTTY = true;
    stream.setRawMode = (params) => {
        logMessage(`setRawMode (does nothing): ${util.inspect(params)}`);
    };
    stream.on('error', (err) => {
        logMessage(`Stream error: ${util.inspect(err)}`);
    });

    const channel = users.get(clientId).getChannel(channelId);
    channel.makeScreen();
    channel.setupXterm();
    channel.greet();
}

server = new Server({
    hostKeys: [fs.readFileSync('./key.private')],
    debug: (message) => {
        logMessage(`Debug: ${message}`);
    }
}, (client) => {
    const clientId = nanoid();
    users.set(clientId, new Client(clientId, client));

    logMessage(`Client connected: ${client.ip}`);
    client.on('authentication', (ctx) => {
        users.get(clientId).name = ctx.username;

        if (ctx.method === 'password' && ctx.username === allowedUser.toString() && ctx.password === allowedPassword.toString()) {
            logMessage(`User ${ctx.username} authenticated`);
            ctx.accept();
        } else {
            logMessage(`User ${ctx.username} not authenticated`);
            ctx.reject();
        }
    });
    client.on('error', (err) => {
        logMessage(`Client error: ${util.inspect(err)}`);
    });
    client.on('ready', () => {
        client.on('session', (accept, reject) => {
            let channelId = nanoid();

            let channelState = {
                stream: undefined,
                name: users.get(clientId).name,
                rows: 24,
                columns: 80,
                terminal: 'ansi',
                commandLineBuffer: ''
            };

            const session = accept();
            session.on('window-change', (accept, reject, info) => {
                if (info.rows < 1 || info.cols < 1)
                    return reject();

                if (clientId && channelId) {
                    let channel = users.get(clientId).getChannel(channelId);
                    channel.resize(info.rows, info.cols);
                }
                accept && accept();
            });
            session.on('pty', (accept, reject, info) => {
                if (info.term === 'invalid')
                    return reject();

                if (info.rows < 1 || info.cols < 1)
                    return reject();

                channelState.rows = info.rows;
                channelState.columns = info.cols;
                channelState.terminal = info.term;
                logMessage(`PTY requested: ${util.inspect(info)}`);
                accept && accept();
            });
            session.once('shell', (accept, reject) => {
                logMessage('Shell requested');
                const stream = accept();
                shellEmulator(clientId, channelId, stream, client, channelState);
            });
            session.on('exec', (accept, reject, info) => {
                logMessage(`Command: ${info.command}`);
                reject();
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