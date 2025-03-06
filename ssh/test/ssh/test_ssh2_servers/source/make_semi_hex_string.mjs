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

export { makeSemiHexString };