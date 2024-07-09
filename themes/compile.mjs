import less from 'less';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url); // get the resolved path to the file
const __dirname = path.dirname(__filename); // get the name of the directory

async function compileLess(inputFile, outputFile, themeName, baseThemeName) {
    const CUSTOM_THEME_METADATA = `.sapThemeMetaData-Base-baseLib {
    background-image: url('data:text/plain;utf-8, { "Path": "Base.baseLib.${themeName}.css_variables", "Extends": ["${baseThemeName}","baseTheme"]}');
}`;

    const parentOutputDir = path.dirname(outputFile);
    if (!fs.existsSync(parentOutputDir)) {
        fs.mkdirSync(parentOutputDir, { recursive: true });
    }

    try {
        const lessData = await fs.promises.readFile(inputFile, 'utf-8');

        const { css } = await less.render(lessData, {
            filename: inputFile
        });
        const output = `${CUSTOM_THEME_METADATA} ${css}`;

        await fs.promises.writeFile(outputFile, output, { encoding: 'utf8', flag: 'w' });
        console.log(`Successfully compiled Less file ${inputFile} to CSS file ${outputFile} with base theme ${baseThemeName}`);

    } catch (error) {
        console.error('Error compiling Less:', error);
    }
}

const destination = process.argv[2];

const directories = fs.readdirSync(__dirname, { withFileTypes: true })
    .filter(dirent => dirent.isDirectory())
    .map(dirent => dirent.name);

console.log(`Compiling themes ${directories.join(', ')} to ${destination}`);
for (const directory of directories) {
    const inputFile = path.join(__dirname, directory, 'theme.less');
    const options = JSON.parse(fs.readFileSync(path.join(__dirname, directory, 'options.json'), 'utf-8'));
    const outputFile = path.join(destination, directory, `css_variables.css`);
    compileLess(inputFile, outputFile, directory, options.baseTheme);
}