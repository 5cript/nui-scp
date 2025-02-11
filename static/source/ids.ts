declare const validChannelId: unique symbol;

type ChannelId = string & {
    [validChannelId]: true;
}

export {
    ChannelId
}