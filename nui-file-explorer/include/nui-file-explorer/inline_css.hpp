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
    display: flex;;
    flex-direction: row;
    flex-wrap: wrap;
    align-content: center;
    align-items: center;
    justify-content: flex-start;
    gap: 5px;
    padding: 5px;
    width: 100%;
    font-size: var(--nui-file-grid-head-font-size);
    border-bottom: 1px solid var(--nui-file-grid-border-color);
}

.nui-file-grid-head-dropdown-text {
    padding-right: 5px;
}

.nui-file-grid-head-dropdown {
    height: 32px;
    width: auto;
    padding: 0 4px;
    display: flex;
    align-items: center;
    border-radius: var(--nui-file-grid-border-radius);
    background-color: var(--nui-file-grid-dropdown-background);
    cursor: pointer;
    border: var(--nui-file-grid-border-width) solid var(--nui-file-grid-border-color);
    white-space: nowrap;
    overflow: hidden;
    color: var(--nui-file-grid-dropdown-color);
    text-overflow: ellipsis;
}

.nui-file-grid-head-dropdown-button-arrow {
    width: 0;
    height: 0;
    border-left: 7px solid transparent;
    border-right: 7px solid transparent;
    border-top: 7px solid var(--nui-file-grid-arrow-color);
}

.nui-file-grid-filter {
    display: flex;
    align-items: center;

    img {
        width: 32px;
        height: 32px;
        padding: 4px;
    }

    input {
        background-color: var(--nui-file-grid-filter-background);
        color: var(--nui-file-grid-filter-color);
        border-radius: var(--nui-file-grid-border-radius);
        padding: 4px;
        height: 32px;
        width: 256px;
    }
}

// @endinline
#endif
// clang-format on
