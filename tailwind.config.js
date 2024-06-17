require('dotenv').config()

const inDevelopment = [
    `${process.env.SOURCE_ROOT}/frontend/**/*.{cpp,hpp}`,
    `${process.env.SOURCE_ROOT}/static/**/*.{html,js}`
]

const inProduction = [
    `${process.env.NUI_PROJECT_ROOT}/frontend/**/*.{cpp,hpp}`,
    `${process.env.NUI_PROJECT_ROOT}/static/**/*.{html,js}`
]

const files = process.env.NODE_ENV === 'development' ? inDevelopment : inProduction;

module.exports = {
    darkMode: 'selector',
    content: {
        files: [...files],
    },
    theme: {
        extend: {},
    },
    plugins: [],
}