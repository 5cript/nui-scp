import path from 'path';
import { fileURLToPath } from 'url';
import webpack from 'webpack';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

export default {
    entry: {
        shell: './shell_emulator.mjs',
        sftp: './sftp_server.mjs'
    },
    plugins: [
        new webpack.IgnorePlugin({
            resourceRegExp: /child_pty$/,
            contextRegExp: /terminal-kit(\/|\\)lib(\/|\\)vte$/
        })
    ],
    output: {
        filename: '[name].mjs',
        path: path.resolve(__dirname, 'dist'),
        devtoolModuleFilenameTemplate: '[absolute-resource-path]'
    },
    experiments: {
        outputModule: true,
        topLevelAwait: true
    },
    target: 'node',
    module: {
        rules: [
            {
                test: /\.node$/,
                loader: "node-loader",
            },
            {
                test: /README/,
                use: 'ignore-loader'
            }
        ]
    },
    devtool: 'inline-source-map'
};