# nui-scp

This project will contain a cross platform SSH and SFTP/SCP client.
Still under construction :construction:

# Todo List

## General UI
- [x] Layouting.
- [ ] Save Layout (done partially).
- [x] Load Layout (on session start only, not afterwards).
- [ ] Custom themes?
- [ ] Custom top bar with moveability and minimize, maximize, close buttons.
- [ ] Accessible logs.
- [x] Multi session support.
- [ ] Unify style of operation queue and rest of the ui.

## Settings UI
- [ ] Add UI for settings.
- [ ] Document settings.

## SSH
- [ ] Key auth.
- [ ] SSH agent support linux.
- [ ] SSH agent support windows?
- [x] Keyboard interactive password auth.
- [ ] GSSAPI auth?
- [ ] KeypassXc database integration?

## Shell
- [x] SSH shell.
- [x] Terminal UIs work.
- [x] Terminal resizing.

## SFTP
- [x] Single file downloads.
- [x] Bulk download.
- [ ] Implement handling of symlinks in bulk downloads.
- [ ] Implement bulk download to archives.
- [ ] Forward download options for bulk downloads.
- [ ] Single file uploads.
- [ ] Bulk uploads.
- [x] Delete file.
- [ ] Rename file.
- [ ] Create file.
- [ ] Create directory.
- [ ] Show file properties.
- [ ] Optimize download speed.
- [ ] Optimize upload speed.
- [ ] Better error reporting, not just logs.
- [x] Pause/Continue operations.
- [x] Cancel operations.

## File View UI
- [x] Grid view.
- [ ] Table view.
- [ ] Split UI into local and remote views.
- [ ] Improve selection of multiple items.
- [ ] Upload drag and drop.
- [ ] Download drag and drop?
- [ ] ...Miscellaneous menu items.