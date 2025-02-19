let logFile;
// logFile = fs.createWriteStream('./log.txt', { flags: 'w' });
logFile = {
    write: (msg) => {
    }
}

function logMessage(verbose, ...args) {
    // logFile.write(`${new Date().toISOString()} - ${message}\n`);
    if (verbose) {
        console.log(...args);
    }
    return {
        trace: () => {
            console.trace();
        }
    }
}

function makeLogger(verbose) {
    return (...args) => {
        return logMessage(verbose, ...args);
    }
}

export { makeLogger };