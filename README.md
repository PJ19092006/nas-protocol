# Network File System over TCP

I made this project to understand file transfer protocols, so eventually decided that why not create my own (ofc a good learning expreince).

---

## What does this do ?

This project allows one machine to act as a **file server**, while another machine can access its files remotely through either:

1. **A command-line client (optional, CMD)** — send explicit filesystem commands such as `LS`, `GET`, `PUT`, `MKDIR`, etc.
2. **A FUSE-mounted filesystem (GUI)** — mount the remote directory locally and interact with it through normal Linux filesystem operations.

---

## Features

- **Multi-Client acceptance using fork()** [for more detail just google it UP!]

- **Dual use case option (client/fuse system):** (client.c) Send explicit commands to server just like a terminal, while for normal GUI use case just go with the fuse.

- **FUSE Filesystem Moun:** the remote directory directly onto your Linux file tree.

- **Custom packet transfers:** *Custom protocol rules* using recursive socket send/recv helpers to completely avoid partial read/write issues common with TCP streams.

- **Safe multi-thread fuse:** takes thread-local storage (__thread) for sockets and mutex locking for multi-threaded safety.


### Custom TCP Protocol

I implimented my own application-layer protocol on top of TCP.

The common networking helpers handle:
* Network byte-order conversion
* Variable-length messages
* Binary payload transfers

# Project Structure

```text
.
├── server.c       # Backend server
├── client.c       # Interactive command-line client
├── fuse_test.c    # FUSE 3 filesystem implementation
├── common.c       # Shared networking/protocol helpers
└── common.h       # Shared declarations and constants
```
*ikik not the best way to structure code to my defence i ain't unemployed & do actually wanna use this code for my nas (not just a project that just exsists)*

---

# 🛠️ Requirements

Currently its only designed for Linux.

Install the required packages:

**fuse**
```bash
sudo apt install build-essential libfuse3-dev fuse3
```
You will need:
* GCC
* FUSE 3
* `libfuse3-dev`

---

# Building

## Build the Server

```bash
gcc server.c common.c -o server
```

## Build the CLI Client
(optional, only when DON'T want a GUI)

```bash
gcc client.c common.c -o client
```

## Build the FUSE Client

```bash
 gcc  fuse_test.c ../common.c $(pkg-config fuse3 --cflags --libs) -o fuse_test
```
---


# Running the System

## 1. Start the Server
DO THIS FIRST
On the machine containing the files you want to access:

```bash
./server
```

The server will listen for incoming TCP connections.

---

## 2. Connect Using the CLI Client (OPTIONAL)

From the client machine:

```bash
./client
```

---

## 3. Mount Using FUSE

Create a mount point:

```bash
mkdir ~/remote_folder
```

Start the FUSE client:

```bash
./fuse_test -f ~/remote_folder
```
use -f flag to keep the terminal running (so that you can see what req are being processed)

The remote filesystem should now appear at:

```text
~/remote_folder
```

### Unmount

```bash
fusermount -u ~/remote_folder
```

---

# 📡 Protocol Specification

Communication between the client and server uses a custom application-layer protocol over TCP.

Requests use text-based command headers followed by optional binary payloads, sizes, offsets, flags, or metadata depending on the operation.

---

# Response Status

Operations use a standardized response status.

| Status             | Value | Meaning                                                 |
| ------------------ | ----: | ------------------------------------------------------- |
| `STATUS_OK`        |   `0` | Operation succeeded                                     |
| `STATUS_ERROR`     |  `-1` | General operation/server error                          |
| `STATUS_NOT_EMPTY` |   `1` | Directory cannot be removed because it contains entries |

---

# Supported Operations

| Operation          | Command                             | Description                                                                                       |
| ------------------ | ----------------------------------- | ------------------------------------------------------------------------------------------------- |
| List Directory     | `LS [dirName]`                      | Lists files and directories inside the specified path.                                            |
| Get File           | `GET [fileName]`                    | Downloads a complete file through the CLI client.                                                 |
| FUSE Read          | `GET [fileName] -f [offset] [size]` | Requests a specific portion of a file for FUSE reads.                                             |
| Put / Write        | `PUT [fileName]`                    | Uploads data or writes a file chunk. Followed by a network-byte-order 64-bit length and raw data. |
| Delete File        | `DELETE [fileName]`                 | Deletes a file.                                                                                   |
| Create Directory   | `MKDIR [dirName]`                   | Creates a directory with permissions `0755`.                                                      |
| Remove Directory   | `RMDIR [dirName]`                   | Removes an empty directory. Returns `STATUS_NOT_EMPTY` if the directory contains entries.         |
| Print Directory    | `PWD`                               | Returns the server's current working directory.                                                   |
| Change Directory   | `CD [dirName]`                      | Changes the server's current working directory.                                                   |
| Get Metadata       | `STAT [fileName]`                   | Retrieves file metadata using `struct stat`.                                                      |
| Truncate           | `TRUNC [fileName]`                  | Changes a file's size to a specified 64-bit value.                                                |
| Rename             | `REN`                               | Renames a file or directory using sequential old/new name payloads.                               |
| Update Timestamps  | `TIME [fileName]`                   | Updates access and modification timestamps.                                                       |
| Change Permissions | `MOD [fileName]`                    | Changes file permissions using a network-ordered `uint32_t` mode.                                 |
| Exit               | `EXIT`                              | Terminates the client session.                                                                    |

---

# File Operations

The protocol supports both complete-file transfers and partial transfers.

For example, a normal CLI download can request:

```text
GET file.txt
```

while the FUSE layer can request only the portion needed for a particular `read()`:

```text
GET file.txt -f [offset] [size]
```

This allows FUSE to perform normal file reads without downloading the entire file every time.

---
