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

function formatDate(date) {
    const options = { weekday: 'short', day: '2-digit', hour: '2-digit', minute: '2-digit', hour12: false };
    return new Intl.DateTimeFormat('en-GB', options).format(date).replace(',', '');
}

const makeLongName = (fakeFsEntry) => {
    const { type, name, target, stat: { mode, uid, gid, userName, groupName, size, atime, mtime, ctime } } = fakeFsEntry;

    /*
        -rwxr-xr-x   1 mjos     staff      348911 Mar 25 14:29 t-filexfer
        1234567890 123 12345678 12345678 12345678 123456789012
    */

    const typeIndicator = (() => {
        switch (type) {
            case 'file': return '-';
            case 'directory': return 'd';
            case 'symlink': return 'l';
            case 'network': return 'n';
            case 'socket': return 's';
            case 'block-device': return 'b';
            case 'character-device': return 'c';
            case 'pipe': return 'p';
            default: return '?';
        }
    })();
    const modeStr = lsAccessMods(mode);

    const blocks = [];
    blocks.push(typeIndicator + modeStr.padEnd(9));

    const linkCount = 0;
    blocks.push(linkCount.toString().padStart(3));

    blocks.push(userName.padEnd(8));
    blocks.push(groupName.padEnd(8));
    blocks.push(size.toString().padStart(8));

    const date = new Date(mtime * 1000);
    blocks.push(formatDate(date));
    blocks.push(name);

    return blocks.join(' ');
}

export { lsAccessMods, makeLongName };