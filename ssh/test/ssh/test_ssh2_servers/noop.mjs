#!/usr/bin/env node

import ssh2 from 'ssh2';
import util from 'node:util';
import blessed from 'blessed';
import fs from 'node:fs';
const { Server, utils: { generateKeyPairSync, parseKey } } = ssh2;

const allowedUser = Buffer.from('test');
const allowedPassword = Buffer.from('test');

const logFile = fs.createWriteStream('./log.txt', { flags: 'w' });

function logMessage(message) {
    logFile.write(`${new Date().toISOString()} - ${message}\n`);
}
logMessage('Server is starting');

function noop(v) { }

const PS1 = '\x1b[1;32m$ \x1b[0m';

const users = [];

// const port = 0;
const port = process.argv[2] ? parseInt(process.argv[2], 10) : 0;

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

const server = new Server({
    hostKeys: [fs.readFileSync('./key.private')],
    debug: (message) => {
        logMessage(`Debug: ${message}`);
    }
}, (client) => {
    let stream;
    let name = 'unknown';

    logMessage(`Client connected: ${client.ip}`);
    client.on('authentication', (ctx) => {
        name = ctx.username;

        if (ctx.method === 'password' && ctx.username === allowedUser.toString() && ctx.password === allowedPassword.toString()) {
            logMessage(`User ${ctx.username} authenticated`);
            ctx.accept();
        } else {
            logMessage(`User ${ctx.username} not authenticated`);
            ctx.reject();
        }
    });
    client.on('ready', () => {
        let rows;
        let cols;
        let term;
        let commandLine = '';
        client.once('session', (accept, reject) => {
            const session = accept();
            session.on('window-change', (accept, reject, info) => {
                rows = info.rows;
                cols = info.cols;
                if (stream) {
                    stream.rows = rows;
                    stream.columns = cols;
                    stream.emit('resize');
                }
                accept && accept();
            });
            session.on('pty', (accept, reject, info) => {
                rows = info.rows;
                cols = info.cols;
                term = info.term;
                logMessage(`PTY requested: ${util.inspect(info)}`);
                accept && accept();
            });
            session.once('shell', (accept, reject) => {
                logMessage('Shell requested');
                stream = accept();
                users.push(stream);

                stream.name = name;
                stream.rows = rows || 24;
                stream.columns = cols || 80;
                stream.isTTY = true;
                stream.setRawMode = noop;
                stream.on('error', noop);

                const screen = new blessed.screen({
                    autoPadding: true,
                    smartCSR: true,
                    program: new blessed.program({
                        input: stream,
                        output: stream
                    }),
                    terminal: term || 'ansi'
                });

                screen.key(['C-c'], () => {
                    client.end();
                    screen.destroy();
                });
                screen.key(['C-d'], () => {
                    client.end();
                    screen.destroy();
                });

                screen.title = 'Hello ' + name;

                // Disable local echo
                //screen.program.attr('invisible', true);

                const handleCommand = (command) => {
                    if (command === 'exit') {
                        screen.program.write('logout\r\n');
                        logMessage('Client exit');
                        client.end();
                        screen.destroy();
                        server.close();
                    }
                }

                stream.on('data', (data) => {
                    logMessage(`Data received: ${makeSemiHexString(data)}`);

                    if (data.length > 1 && data.includes('\r')) {
                        const lines = data.toString().split('\r');
                        lines.forEach((line, index) => {
                            if (index < lines.length - 1) {
                                commandLine += line;
                                logMessage(`Command executed: ${makeSemiHexString(commandLine)}`);
                                handleCommand(commandLine);
                                screen.program.write('\r\n');
                                screen.program.write(PS1);
                                commandLine = '';
                            } else {
                                commandLine += line;
                            }
                        });
                    }

                    if (data.toString().endsWith('\r')) {
                        logMessage(`Command executed: ${makeSemiHexString(commandLine)}`);
                        handleCommand(commandLine.slice(0, -1));
                        screen.program.write('\r\n');
                        screen.program.write(PS1);
                        commandLine = '';
                        return;
                    }

                    if (data.toString() === '\x7f') { // Handle backspace
                        if (commandLine.length > 0) {
                            commandLine = commandLine.slice(0, -1);
                            screen.program.write('\b \b'); // Move cursor back, overwrite with space, move cursor back again
                        }
                    } else if (data.toString() === '\r') {
                        logMessage(`Command executed: ${makeSemiHexString(commandLine)}`);
                        screen.program.write('\r\n');
                        screen.program.write(PS1);
                        handleCommand(commandLine);
                        commandLine = '';
                    } else {
                        commandLine += data.toString();
                    }
                    screen.program.write(data);
                });

                stream.on('resize', () => {
                    screen.program.emit('resize');
                });

                screen.render();
                // XXX This fake resize event is needed for some terminals in order to
                // have everything display correctly
                screen.program.emit('resize');

                screen.program.clear();
                screen.program.write(PS1);
            });
            session.on('exec', (accept, reject, info) => {
                logMessage(`Command: ${info.command}`);
                if (info.command === 'noop') {
                    const stream = accept();
                    stream.stderr.write('noop\n');
                    stream.exit(0);
                    stream.end();
                    server.close();
                } else {
                    reject();
                }
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
