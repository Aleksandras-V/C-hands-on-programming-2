# Welcome Aleksandras IPC file copy

This program copies content from a file to another.
Both files should exist!

## Implemented methods:
- Messages, use : 
    - `ipc_receivefile --messages --file <your dest file>`
    - `ipc_sendfile --messages --file <your source file>`
- Shared memory, use : 
    - `ipc_receivefile --shm --file <your dest file>`
    - `ipc_sendfile --shm --file <your source file>`

- For command information use : `ipc_receivefile --help` and `ipc_sendfile --help`
