#pragma once

// clang-format off
#ifdef NUI_INLINE
// @inline(css, nui-file-grid)

.nui-file-grid-icons {
    display: grid;
}

.nui-file-grid-item-icons {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    padding: 10px;

    & div {
        margin-top: 8px;
        text-align: center;
    }
}

.nui-file-grid-item-icons:hover {
    background-color: var(--nui-file-grid-item-highlight);
}

.nui-file-grid-item-icons:is(.selected) {
    background-color: var(--nui-file-grid-item-highlight);
    border: 1px solid var(--nui-file-grid-border-color);
    border-radius: var(--nui-file-grid-border-color);
}

.nui-file-grid-head {
    height: auto;
    width: 100%;
    border-bottom: 1px solid var(--nui-file-grid-border-color);
}

.nui-file-grid-head-dropdown {
    height: 32px;
    width: auto;
    padding: 4px;
}

.nui-file-grid-head-dropdown-button-arrow {
    width: 0;
    height: 0;
    border-left: 20px solid transparent;
    border-right: 20px solid transparent;

    border-top: 20px solid #f00;
}

.nui-file-grid-filter {
    img {
        width: 32px;
        height: 32px;
        padding: 4px;
    }

    input {
        background-color: var(--nui-file-grid-filter-background);
        color: var(--nui-file-grid-filter-color);
        border-radius: var(--nui-file-grid-filter-border-radius);
        padding: 4px;
        height: 32px;
        width: 256px;
    }
}

// @endinline
#endif
// clang-format on
