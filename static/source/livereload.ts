import RpcClient from 'nui/rpc';

(() => {
    globalThis.reloadStyles = () => {
        const linkElements = document.querySelectorAll('link[rel="stylesheet"]');
        linkElements.forEach(linkElement => {
            const href = linkElement.getAttribute('href');
            if (href && href.includes('liveload')) {
                const parsedHref = new URL(href);
                parsedHref.searchParams.set('v', Date.now().toString());
                linkElement.setAttribute('href', parsedHref.toString());
            }
        });
    }

    RpcClient.register('livereload::reloadStyles', globalThis.reloadStyles);
    const result = RpcClient.call('livereload::ready');
    if (result instanceof RpcClient.UnresolvedError) {
        console.error(result);
    }
})()