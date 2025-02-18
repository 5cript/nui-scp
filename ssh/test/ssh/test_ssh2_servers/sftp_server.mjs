#!/usr/bin/env node


// https://github.com/mscdex/ssh2/blob/master/SFTP.md

import ssh2 from 'ssh2';
import util from 'node:util';
import fs from 'node:fs';
import { nanoid } from 'nanoid';
import minimist from 'minimist';
import { makeLogger } from './source/log.mjs';
import { humanFileSize } from './source/human_file_size.mjs';
import CommandLineInterface from './source/cli.mjs';
import { makeSemiHexString } from './source/make_semi_hex_string.mjs';
import { exit } from 'node:process';
import crypto from 'node:crypto';
import { file, directory, symlink, finalizeFakeFs } from './source/fake_file_system.mjs';
const { Server, sftp } = ssh2;
const { OPEN_MODE, STATUS_CODE } = ssh2.utils.sftp;

const args = minimist(process.argv.slice(2), {
    alias: {
        p: 'port',
        v: 'verbose'
    },
    default: {
        port: 0,
        verbose: false
    }
});

const port = args.port;
const logMessage = makeLogger(args.verbose);

const allowedUser = Buffer.from('test');
const allowedPassword = Buffer.from('test');

function checkValue(input, allowed) {
    const autoReject = (input.length !== allowed.length);
    if (autoReject) {
        // Prevent leaking length information by always making a comparison with the
        // same input when lengths don't match what we expect ...
        allowed = input;
    }
    const isMatch = crypto.timingSafeEqual(input, allowed);
    return (!autoReject && isMatch);
}

const fakeFilesystem = finalizeFakeFs({
    ...directory({ name: '' }, [
        directory({ name: 'home' }, [
            directory({ name: 'test' }, [
                directory({ name: 'Documents' }, [
                    file('doc1.txt', 'Document 1 content'),
                    file('doc2.txt', 'Document 2 content')
                ]),
                directory({ name: 'Downloads' }, [
                    file('file1.zip', 'Fake zip content'),
                    file('file2.tar.gz', 'Fake tar.gz content')
                ]),
                directory({ name: 'Pictures' }, [
                    file('image1.png', 'Fake image content'),
                    file('image2.jpg', 'Fake image content')
                ]),
                directory({ name: 'Music' }, [
                    file('song1.mp3', 'Fake song content'),
                    file('song2.wav', 'Fake song content')
                ]),
                directory({ name: 'Videos' }, [
                    file('video1.mp4', 'Fake video content'),
                    file('video2.avi', 'Fake video content')
                ]),
                directory({ name: 'Desktop' }, [
                    file('shortcut1.lnk', 'Fake shortcut content'),
                    file('shortcut2.lnk', 'Fake shortcut content')
                ]),
                symlink('link_to_doc1.txt', '/home/user/Documents/doc1.txt'),
                file('file1.txt', 'Fake file content'),
                file('file2.txt', 'Fake file content'),
            ])
        ]),
        directory({ name: 'etc' }, [
            file('passwd', 'Fake passwd content'),
            file('hosts', 'Fake hosts content')
        ]),
        directory({ name: 'var' }, [
            directory({ name: 'log' }, [
                file('syslog', 'Fake syslog content'),
                file('auth.log', 'Fake auth.log content')
            ])
        ])
    ])
});

logMessage(fakeFilesystem);
logMessage(`Find /home/test/Documents: ${util.inspect(fakeFilesystem.find('/home/test/Documents'))}`);

const defaultPath = '/home/test';

const server = new Server({
    hostKeys: [fs.readFileSync('key.private')],
    debug: (message) => {
        logMessage(`Debug: ${message}`);
    }
}, (client) => {
    logMessage('Client connected!');

    client.on('authentication', (ctx) => {
        let allowed = true;
        if (!checkValue(Buffer.from(ctx.username), allowedUser))
            allowed = false;

        logMessage(`User and password: ${ctx.username} ${ctx.password}`);
        logMessage(`Method: ${ctx.method}`);

        switch (ctx.method) {
            case 'password':
                if (!checkValue(Buffer.from(ctx.password), allowedPassword))
                    return ctx.reject();
                break;
            default:
                return ctx.accept();
        }
    });

    client.on('ready', () => {
        logMessage('Client authenticated!');

        client.on('session', (accept, reject) => {
            const session = accept();

            session.on('sftp', (accept, reject) => {
                logMessage('Client SFTP session');

                const openFiles = new Map();
                let handleCount = 0;
                const sftpStream = accept();
                sftpStream.on('OPEN', (reqid, filename, flags, attrs) => {
                    logMessage('OPEN', filename, flags, attrs);
                    const openFiles = new Map();

                    if (filename !== '/tmp/foo.txt' || !(flags & OPEN_MODE.WRITE))
                        return sftpStream.status(reqid, STATUS_CODE.FAILURE);

                    const handle = Buffer.alloc(4);
                    openFiles.set(handleCount, true);
                    handle.writeUInt32BE(handleCount++, 0);

                    logMessage('Opening file for write')
                    sftpStream.handle(reqid, handle);
                });

                sftpStream.on('WRITE', (reqid, handle, offset, data) => {
                    if (handle.length !== 4
                        || !openFiles.has(handle.readUInt32BE(0))) {
                        return sftpStream.status(reqid, STATUS_CODE.FAILURE);
                    }

                    // Fake the write operation
                    sftpStream.status(reqid, STATUS_CODE.OK);

                    logMessage('Write to file at offset ${offset}: ${inspect(data)}');
                });

                sftpStream.on('CLOSE', (reqid, handle) => {
                    if (handle.length !== 4
                        || !openFiles.has(handle.readUInt32BE(0))) {
                        return sftpStream.status(reqid, STATUS_CODE.FAILURE);
                    }

                    openFiles.delete(handle.readUInt32BE(0));
                    sftpStream.status(reqid, STATUS_CODE.OK);

                    logMessage('File closed');
                });

                sftpStream.on('REALPATH', (reqid, path) => {
                    // const name = [{
                    //     filename: '/tmp/foo.txt',
                    //     longname: '-rwxrwxrwx 1 foo foo 3 Dec 8 2009 foo.txt',
                    //     attrs: {}
                    // }];
                    // sftpStream.name(reqid, name);
                    if (path === '.') {
                        path = defaultPath;
                    }

                    const result = fakeFilesystem.find(path);

                    logMessage('REALPATH', path, result);

                    if (result === undefined) {
                        return sftpStream.status(reqid, STATUS_CODE.FAILURE);
                    }

                    const name = [{
                        filename: path,
                        attrs: result.stat
                    }];

                    logMessage(name);

                    sftpStream.name(reqid, name);
                });

                sftpStream.on('STAT', (reqid, path) => {
                    logMessage('STAT', path);
                    sftpStream.attrs(reqid, { mode: 0o644, uid: 0, gid: 0, size: 0, atime: 0, mtime: 0 });
                });

                sftpStream.on('OPENDIR', (reqid, path) => {
                    logMessage('OPENDIR', path);
                    const handle = Buffer.alloc(4);
                    openFiles.set(handleCount, path);
                    handle.writeUInt32BE(handleCount++, 0);

                    logMessage(`Opening directory with handle ${handleCount}`);
                    sftpStream.handle(reqid, handle);
                });

                sftpStream.on('READDIR', (reqid, handle) => {
                    if (handle.length !== 4
                        || !openFiles.has(handle.readUInt32BE(0))) {
                        return sftpStream.status(reqid, STATUS_CODE.FAILURE);
                    }

                    const path = openFiles.get(handle.readUInt32BE(0));
                    const result = fakeFilesystem.find(path);

                    if (result === undefined || result.type !== 'directory') {
                        return sftpStream.status(reqid, STATUS_CODE.FAILURE);
                    }

                    const entries = result.children.map((child) => {
                        return {
                            filename: child.name,
                            longname: `-${child.type === 'directory' ? 'd' : '-'}rwxrwxrwx 1 foo foo ${humanFileSize(child.stat.size)} Dec 8 2009 ${child.name}`,
                            attrs: child.stat
                        };
                    });

                    logMessage('READDIR', path, entries);

                    sftpStream.name(reqid, entries);
                });

                sftpStream.on('CLOSE', (reqid, handle) => {
                    if (handle.length !== 4
                        || !openFiles.has(handle.readUInt32BE(0))) {
                        return sftpStream.status(reqid, STATUS_CODE.FAILURE);
                    }

                    openFiles.delete(handle.readUInt32BE(0));
                    sftpStream.status(reqid, STATUS_CODE.OK);
                });

                sftpStream.on('REMOVE', (reqid, path) => {
                    logMessage('REMOVE', path);
                    sftpStream.status(reqid, STATUS_CODE.OK);
                });

                sftpStream.on('MKDIR', (reqid, path) => {
                    logMessage('MKDIR', path);
                    sftpStream.status(reqid, STATUS_CODE.OK);
                });

                sftpStream.on('RMDIR', (reqid, path) => {
                    logMessage('RMDIR', path);
                    sftpStream.status(reqid, STATUS_CODE.OK);
                });

                sftpStream.on('RENAME', (reqid, oldPath, newPath) => {
                    logMessage('RENAME', oldPath, newPath);
                    sftpStream.status(reqid, STATUS_CODE.OK);
                });

                sftpStream.on('READLINK', (reqid, path) => {
                    logMessage('READLINK', path);
                    sftpStream.name(reqid, [{ filename: path, longname: 'foo', attrs: {} }]);
                });

                sftpStream.on('SYMLINK', (reqid, targetPath, linkPath) => {
                    logMessage('SYMLINK', targetPath, linkPath);
                    sftpStream.status(reqid, STATUS_CODE.OK);
                });

                sftpStream.on('SETSTAT', (reqid, path, attrs) => {
                    logMessage('SETSTAT', path, attrs);
                    sftpStream.status(reqid, STATUS_CODE.OK);
                });

                sftpStream.on('FSETSTAT', (reqid, handle, attrs) => {
                    logMessage('FSETSTAT', handle, attrs);
                    sftpStream.status(reqid, STATUS_CODE.OK);
                });

                sftpStream.on('FSTAT', (reqid, handle) => {
                    logMessage('FSTAT', handle);
                    sftpStream.attrs(reqid, { mode: 0o644, uid: 0, gid: 0, size: 0, atime: 0, mtime: 0 });
                });

                sftpStream.on('LSTAT', (reqid, path) => {
                    logMessage('LSTAT', path);
                    sftpStream.attrs(reqid, { mode: 0o644, uid: 0, gid: 0, size: 0, atime: 0, mtime: 0 });
                });
            });
        });
    });


    client.on('close', () => {
        logMessage('Client disconnected!');
    });
});

server.listen(port, '::', () => {
    const address = server.address();
    logMessage(`Listening on ${address.address}:${address.port}`);
    console.log(JSON.stringify({ port: server.address().port }));
})

const cli = new CommandLineInterface();

cli.on('exit', () => {
    server.close();
});