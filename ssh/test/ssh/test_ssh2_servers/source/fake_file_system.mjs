import path from 'node:path';

const defaultStat = {
    mode: 0o644,
    uid: 1000,
    gid: 1000,
    size: 0,
    atime: 0,
    mtime: 0,
    ctime: 0,
    birthtime: 0
}


const file = (name, contentString, stat) => {
    return {
        type: 'file',
        name: name,
        stat: {
            ...defaultStat,
            size: contentString.length,
            ...(stat || {})
        }
    }
}

const directory = ({ name, stat }, children) => {
    return {
        type: 'directory',
        name: name,
        stat: {
            ...defaultStat,
            mode: 0o755,
            ...(stat || {})
        },
        children: children || [],
        find: function (pathString) {
            const pathParts = pathString.split('/').filter(part => part.length > 0);

            if (pathParts.length !== 0) {
                const currentSegment = pathParts[0];
                if (currentSegment === '..') {
                    if (this.parent === undefined) {
                        return undefined;
                    }
                    const remainingPath = pathParts.slice(1).join('/');
                    return this.parent.find(remainingPath);
                }
                if (currentSegment === '.') {
                    const remainingPath = pathParts.slice(1).join('/');
                    return this.find(remainingPath);
                }
                const child = this.children.find(child => child.name === currentSegment);

                if (child === undefined)
                    return undefined;

                const remainingPath = pathParts.slice(1).join('/');
                if (remainingPath === '')
                    return child;

                return child.find(remainingPath);
            }
        }
    }
}

const symlink = (name, target, stat) => {
    if (stat === undefined)
        stat = {}

    return {
        type: 'symlink',
        name: name,
        target: target,
        stat: {
            ...defaultStat,
            mode: 0o777,
            ...(stat || {})
        }
    }
}

// Gives every child directory a reference to its parent directory
const fillFakeFsParents = (root) => {
    const fillParent = (parent, child) => {
        if (child.type === 'directory') {
            child.parent = parent;
            child.children.forEach(c => fillParent(child, c));
        }
    }

    fillParent(undefined, root);
}

const finalizeFakeFs = (root) => {
    fillFakeFsParents(root);
    return root;
}

export { file, directory, symlink, finalizeFakeFs };