#include "../common.h"
#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <sys/time.h>
#include <pthread.h>

#define PORT 5000
#define SERVER_IP "10.42.0.250"
#define TIMEOUT_SEC 5 

typedef struct {
    int sock;
    pthread_mutex_t lock;
} ThreadSocket;

static __thread ThreadSocket thread_slot = { .sock = -1, .lock = PTHREAD_MUTEX_INITIALIZER };

static int get_thread_socket() {
    if (thread_slot.sock != -1) return thread_slot.sock;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return -1;
    }

    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));

    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("connect (reconnect failed)");
        close(sock);
        return -1;
    }

    thread_slot.sock = sock;
    return thread_slot.sock;
}


static void invalidate_thread_socket() {
    if (thread_slot.sock != -1) {
        close(thread_slot.sock);
        thread_slot.sock = -1;
    }
}

static int my_getattr(const char *path, struct stat *st, struct fuse_file_info *fi) {
    printf("getattr called: %s\n", path);
    memset(st, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
        return 0;
    }

    const char *fileName = path + 1;
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "%s %s", STAT_CALL, fileName);

    // Retry loop for automatic reconnection on failure
    for (int attempt = 0; attempt < 2; attempt++) {
        int sockFd = get_thread_socket();
        if (sockFd == -1) return -EIO;

        pthread_mutex_lock(&thread_slot.lock);

        if (send_msg(sockFd, request, strlen(request)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        Response res;
        if (recvHelper(sockFd, &res, sizeof(res)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        if (res.status != STATUS_OK) {
            pthread_mutex_unlock(&thread_slot.lock);
            return -ENOENT;
        }

        FileStat file;
        if (recvHelper(sockFd, &file, sizeof(file)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        pthread_mutex_unlock(&thread_slot.lock);

        st->st_mode = file.mode;
        st->st_nlink = file.nlink;
        st->st_size = file.size;
        st->st_atim = file.atime;
        st->st_mtim = file.mtime;
        return 0;
    }

    return -EIO;
}

static int my_mkdir(const char *path, mode_t mode) {
    printf("mkdir called: %s\n", path);
    const char *dirName = path + 1;
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "%s %s", CREATE_DIR, dirName);

    for (int attempt = 0; attempt < 2; attempt++) {
        int sockFd = get_thread_socket();
        if (sockFd == -1) return -EIO;

        pthread_mutex_lock(&thread_slot.lock);
        if (send_msg(sockFd, request, strlen(request)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        Response res;
        if (recvHelper(sockFd, &res, sizeof(res)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }
        pthread_mutex_unlock(&thread_slot.lock);

        if (res.status != STATUS_OK) return -EACCES;
        return 0;
    }
    return -EIO;
}

static int my_rmdir(const char *path) {
    printf("rmdir called: %s\n", path);
    const char *dirName = path + 1;
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "%s %s", DELETE_DIR, dirName);

    for (int attempt = 0; attempt < 2; attempt++) {
        int sockFd = get_thread_socket();
        if (sockFd == -1) return -EIO;

        pthread_mutex_lock(&thread_slot.lock);
        if (send_msg(sockFd, request, strlen(request)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        Response res;
        if (recvHelper(sockFd, &res, sizeof(res)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }
        pthread_mutex_unlock(&thread_slot.lock);

        if (res.status == STATUS_NOT_EMPTY) return -ENOTEMPTY; // If you define a specific status
        if (res.status != STATUS_OK) return -ENOENT;
        return 0;
    }
    return -EIO;
}

static int my_unlink(const char *path) {
    printf("unlink called: %s\n", path);
    const char *fileName = path + 1;
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "%s %s", DELETE_FILE, fileName);

    for (int attempt = 0; attempt < 2; attempt++) {
        int sockFd = get_thread_socket();
        if (sockFd == -1) return -EIO;

        pthread_mutex_lock(&thread_slot.lock);
        if (send_msg(sockFd, request, strlen(request)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        Response res;
        if (recvHelper(sockFd, &res, sizeof(res)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }
        pthread_mutex_unlock(&thread_slot.lock);

        if (res.status != STATUS_OK) return -ENOENT;
        return 0;
    }
    return -EIO;
}

static int my_opendir(const char *path, struct fuse_file_info *fi) {
    printf("opendir called: %s\n", path);
    return 0;
}

static int my_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    printf("create called: %s\n", path);
    const char *fileName = path + 1;
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "%s %s", PUT_CALL, fileName);

    for (int attempt = 0; attempt < 2; attempt++) {
        int sockFd = get_thread_socket();
        if (sockFd == -1) return -EIO;

        pthread_mutex_lock(&thread_slot.lock);
        if (send_msg(sockFd, request, strlen(request)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        uint64_t zeroLen = 0;
        uint64_t networkLen = htobe64(zeroLen);
        if (sendRecursively(sockFd, &networkLen, sizeof(networkLen)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        Response res;
        if (recvHelper(sockFd, &res, sizeof(res)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }
        pthread_mutex_unlock(&thread_slot.lock);

        if (res.status != STATUS_OK) return -EIO;
        return 0;
    }
    return -EIO;
}

static int my_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    printf("write called: %s | size=%zu | offset=%ld\n", path, size, offset);
    const char *fileName = path + 1;
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "%s %s", PUT_CALL, fileName);

    for (int attempt = 0; attempt < 2; attempt++) {
        int sockFd = get_thread_socket();
        if (sockFd == -1) return -EIO;

        pthread_mutex_lock(&thread_slot.lock);
        if (send_msg(sockFd, request, strlen(request)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        uint64_t networkSize = htobe64(size);
        if (sendRecursively(sockFd, &networkSize, sizeof(networkSize)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        if (sendRecursively(sockFd, buf, size) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        Response res;
        if (recvHelper(sockFd, &res, sizeof(res)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }
        pthread_mutex_unlock(&thread_slot.lock);

        if (res.status != STATUS_OK) return -EIO;
        return (int)size;
    }
    return -EIO;
}

static int my_truncate(const char *path, off_t size, struct fuse_file_info *fi) {
    printf("truncate called: %s | size=%ld\n", path, size);
    const char *fileName = path + 1;
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "%s %s", TRUNCATE_CALL, fileName);

    for (int attempt = 0; attempt < 2; attempt++) {
        int sockFd = get_thread_socket();
        if (sockFd == -1) return -EIO;

        pthread_mutex_lock(&thread_slot.lock);
        if (send_msg(sockFd, request, strlen(request)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        uint64_t networkSize = htobe64(size);
        if (sendRecursively(sockFd, &networkSize, sizeof(networkSize)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        Response res;
        if (recvHelper(sockFd, &res, sizeof(res)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }
        pthread_mutex_unlock(&thread_slot.lock);

        if (res.status != STATUS_OK) return -EIO;
        return 0;
    }
    return -EIO;
}

static int my_rename(const char *from, const char *to, unsigned int flags) {
    printf("rename called: from %s to %s\n", from, to);
    const char *oldName = from + 1;
    const char *newName = to + 1;
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "%s", RENAME_CALL);

    for (int attempt = 0; attempt < 2; attempt++) {
        int sockFd = get_thread_socket();
        if (sockFd == -1) return -EIO;

        pthread_mutex_lock(&thread_slot.lock);
        if (send_msg(sockFd, request, strlen(request)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        if (send_msg(sockFd, (char *)oldName, strlen(oldName)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }
        if (send_msg(sockFd, (char *)newName, strlen(newName)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        Response res;
        if (recvHelper(sockFd, &res, sizeof(res)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }
        pthread_mutex_unlock(&thread_slot.lock);

        if (res.status != STATUS_OK) return -EIO;
        return 0;
    }
    return -EIO;
}

static int my_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi) {
    printf("utimens called: %s\n", path);
    const char *fileName = path + 1;
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "%s %s", UTIMENS_CALL, fileName);

    for (int attempt = 0; attempt < 2; attempt++) {
        int sockFd = get_thread_socket();
        if (sockFd == -1) return -EIO;

        pthread_mutex_lock(&thread_slot.lock);
        if (send_msg(sockFd, request, strlen(request)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }
        if (sendRecursively(sockFd, tv, sizeof(struct timespec) * 2) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        Response res;
        if (recvHelper(sockFd, &res, sizeof(res)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }
        pthread_mutex_unlock(&thread_slot.lock);

        if (res.status != STATUS_OK) return -EIO;
        return 0;
    }
    return -EIO;
}

static int my_chmod(const char *path, mode_t mode, struct fuse_file_info *fi) {
    printf("chmod called: %s | mode=%o\n", path, mode);
    const char *fileName = path + 1;
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "%s %s", CHMOD_CALL, fileName);

    for (int attempt = 0; attempt < 2; attempt++) {
        int sockFd = get_thread_socket();
        if (sockFd == -1) return -EIO;

        pthread_mutex_lock(&thread_slot.lock);
        if (send_msg(sockFd, request, strlen(request)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        uint32_t networkMode = htonl(mode);
        if (sendRecursively(sockFd, &networkMode, sizeof(networkMode)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        Response res;
        if (recvHelper(sockFd, &res, sizeof(res)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }
        pthread_mutex_unlock(&thread_slot.lock);

        if (res.status != STATUS_OK) return -EIO;
        return 0;
    }
    return -EIO;
}

static int my_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    printf("readdir called: %s\n", path);
    
    filler(buffer, ".", NULL, 0, 0);
    filler(buffer, "..", NULL, 0, 0);

    char request[BUFFER_SIZE];
    if (strcmp(path, "/") == 0) {
        snprintf(request, sizeof(request), "%s .", LIST_ALL);
    } else {
        const char *dirName = path + 1;
        snprintf(request, sizeof(request), "%s %s", LIST_ALL, dirName);
    }

    for (int attempt = 0; attempt < 2; attempt++) {
        int sockFd = get_thread_socket();
        if (sockFd == -1) return -EIO;

        pthread_mutex_lock(&thread_slot.lock);
        if (send_msg(sockFd, request, strlen(request)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        uint32_t totalFiles = getHeader(sockFd);
        if (totalFiles == 0 && errno != 0) { // Check if getHeader failed due to timeout/disconnect
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        int failed = 0;
        for (uint32_t i = 0; i < totalFiles; i++) {
            char *filename = get_msg(sockFd);
            if (filename == NULL) {
                failed = 1;
                break;
            }
            filler(buffer, filename, NULL, 0, 0);
            free(filename);
        }

        pthread_mutex_unlock(&thread_slot.lock);

        if (failed) {
            invalidate_thread_socket();
            continue;
        }

        return 0;
    }
    return -EIO;
}

static int my_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
    printf("read called: %s | size=%zu | offset=%ld\n", path, size, offset);
    const char *fileName = path + 1;
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "%s %s %s %ld %zu", GET_CALL, fileName, FUSE_FLAG, offset, size);

    for (int attempt = 0; attempt < 2; attempt++) {
        int sockFd = get_thread_socket();
        if (sockFd == -1) return -EIO;

        pthread_mutex_lock(&thread_slot.lock);
        if (send_msg(sockFd, request, strlen(request)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        Response res;
        if (recvHelper(sockFd, &res, sizeof(res)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }
        if (res.status != STATUS_OK) {
            pthread_mutex_unlock(&thread_slot.lock);
            return -EIO;
        }

        uint64_t networkSize;
        if (recvHelper(sockFd, &networkSize, sizeof(networkSize)) == -1) {
            pthread_mutex_unlock(&thread_slot.lock);
            invalidate_thread_socket();
            continue;
        }

        uint64_t chunkSize = be64toh(networkSize);
        if (chunkSize > (uint64_t)size) {
            pthread_mutex_unlock(&thread_slot.lock);
            return -EIO;
        }

        // Safely loop or read the exact chunk bytes
        size_t totalRead = 0;
        while (totalRead < chunkSize) {
            ssize_t bytesRead = recvHelper(sockFd, buffer + totalRead, chunkSize - totalRead);
            if (bytesRead <= 0) {
                pthread_mutex_unlock(&thread_slot.lock);
                invalidate_thread_socket();
                break;
            }
            totalRead += bytesRead;
        }

        if (totalRead != chunkSize) {
            continue;
        }

        pthread_mutex_unlock(&thread_slot.lock);
        return (int)chunkSize;
    }
    return -EIO;
}

static int my_open(const char *path, struct fuse_file_info *fi) {
    printf("open called: %s\n", path);
    int mode = fi->flags & O_ACCMODE;
    if (mode != O_RDONLY && mode != O_WRONLY && mode != O_RDWR) {
        return -EACCES;
    }
    return 0;
}

static const struct fuse_operations operations = {
    .getattr = my_getattr,
    .readdir = my_readdir,
    .read = my_read,
    .open = my_open,
    .mkdir = my_mkdir,
    .rmdir = my_rmdir,
    .unlink = my_unlink,
    .opendir = my_opendir,
    .create = my_create,
    .write = my_write,
    .utimens = my_utimens,
    .truncate = my_truncate,
    .rename = my_rename,
    .chmod = my_chmod,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &operations, NULL);
}