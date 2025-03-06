import fs from 'fs';
import util from 'util';

function logMessage(verbose, logFile, ...args) {
    // logFile.write(`${new Date().toISOString()} - ${message}\n`);
    if (verbose) {
        //console.log(...args);
    }
    if (logFile) {
        for (let arg of args) {
            if (arg instanceof Error) {
                logFile.write(arg.stack);
            }
            else if (typeof arg === 'object') {
                logFile.write(util.inspect(arg, { depth: 10 }));
            }
            else if (typeof arg === 'string') {
                logFile.write(arg);
                logFile.write('\n');
            } else {
                logFile.write(arg.toString());
                logFile.write('\n');
            }
        }
    }
    return {
        trace: () => {
            if (verbose) {
                console.trace();
            }
        },
        end: () => {
            if (logFile) {
                logFile.end();
                logFile = undefined;
            }
        }
    }
}

function makeLogger(verbose, file) {
    let logFile = undefined;
    return (...args) => {
        if (file !== undefined && logFile === undefined) {
            logFile = fs.createWriteStream(file, { flags: 'w' });
        }
        return logMessage(verbose, logFile, ...args);
    }
}

function flushLogger() {
    if (logFile) {
        logFile.end();
    }
}

export { makeLogger };