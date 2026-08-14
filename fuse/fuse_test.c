#include "../common.h"
#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>

#define PORT 5000

static int sockFd = -1;


static const char *file_content = "Hello from my FUSE filesystem!\n";


static int my_getattr(const char *path,struct stat *st,struct fuse_file_info *fi){
    printf("getattr called: %s\n", path);

    memset(st, 0, sizeof(struct stat));

    // Root directory
    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
        return 0;
    }

    const char *fileName = path + 1;

    char request[BUFFER_SIZE];
    snprintf(request,sizeof(request),"%s %s",STAT_CALL,fileName);

    // Sending request 
    if (send_msg(sockFd, request, strlen(request)) == -1) return -EIO;

    // Receive status
    Response res;
    if (recvHelper(sockFd, &res, sizeof(res)) == -1) return -EIO;

    if (res.status != STATUS_OK) return -ENOENT;

    // getting data
    FileStat file;

    if (recvHelper(sockFd, &file, sizeof(file)) == -1) return -EIO;

    // handing over the data recived
    st->st_mode = file.mode;
    st->st_nlink = file.nlink;
    st->st_size = file.size;

    return 0;
}



static int connect_server(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == -1) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server = {0};

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock,(struct sockaddr *)&server,sizeof(server)) == -1) {
        perror("connect");
        close(sock);
        return -1;
    }

    return sock;
}

static int my_readdir(const char *path,void *buffer,fuse_fill_dir_t filler,off_t offset,struct fuse_file_info *fi,enum fuse_readdir_flags flags){
    printf("readdir called: %s\n", path);

    if (strcmp(path, "/") != 0) return -ENOENT;

    // Ask NAS server for directory listing
    if (send_msg(sockFd, LIST_ALL, strlen(LIST_ALL)) == -1) return -EIO;

    // Server first sends number of files
    uint32_t totalFiles = getHeader(sockFd);

    printf("Server says there are %u files\n", totalFiles);

    // Receive every filename
    for (uint32_t i = 0; i < totalFiles; i++) {

        char *filename = get_msg(sockFd);

        if (filename == NULL) return -EIO;

        printf("received filename: %s\n", filename);

        filler(buffer, filename, NULL, 0, 0);

        free(filename);
    }

    return 0;
}


static int my_read(const char *path,char *buffer,size_t size,off_t offset,struct fuse_file_info *fi){
    printf("read called: %s | size=%zu | offset=%ld\n",path, size, offset);

    const char *fileName = path + 1;

    char request[BUFFER_SIZE];

    snprintf(request,sizeof(request),"%s %s %s %ld %zu",GET_CALL,fileName,FUSE_FLAG,offset,size);
    printf("Sending: %s\n", request);
    if (send_msg(sockFd, request, strlen(request)) == -1)return -EIO;

    // gettig status
    Response res;
    if (recvHelper(sockFd, &res, sizeof(res)) == -1)return -EIO;
    if (res.status != STATUS_OK)return -EIO;

    // file size 
    uint64_t networkSize;
    if (recvHelper(sockFd, &networkSize, sizeof(networkSize)) == -1) return -EIO;

    uint64_t chunkSize = be64toh(networkSize);
    if (chunkSize > size)return -EIO; // extra check

    // sending data to buffer 
    if (recvHelper(sockFd, buffer, chunkSize) == -1)return -EIO;

    return (int)chunkSize;
}


static int my_open(const char *path,struct fuse_file_info *fi){
    printf("open called: %s\n", path);

    if ((fi->flags & O_ACCMODE) != O_RDONLY)return -EACCES;

    return 0;
}


static const struct fuse_operations operations = {
    .getattr = my_getattr,
    .readdir = my_readdir,
    .read = my_read,
    .open = my_open,
};


int main(int argc, char *argv[]){
    sockFd = connect_server();

    if (sockFd == -1) return -1;
    int ret = fuse_main(argc, argv, &operations, NULL);

    close(sockFd);
    return ret;
}