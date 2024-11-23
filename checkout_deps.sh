#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

is_git_repo() {
    if [ -d "$1/.git" ]; then
        return 0
    else
        return 1
    fi
}

checkout_or_update() {
    if is_git_repo "$1"; then
        git -C "$1" pull --ff-only
    else
        git clone "$2" "$1"
    fi
}

cd "$SCRIPT_DIR/dependencies" || exit 1

checkout_or_update "OS-Folder-Icons" "https://github.com/shariati/OS-Folder-Icons.git"
checkout_or_update "Nui" "https://github.com/NuiCpp/Nui.git"
checkout_or_update "roar" "https://github.com/5cript/roar.git"