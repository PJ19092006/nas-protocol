#ifndef COMMON_H
#define COMMON_H

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>  
#include <stdlib.h>
#include <dirent.h> 
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h> 
#include <endian.h>


#define PORT 5000
#define BUFFER_SIZE 1024

// all the calls
#define EXIT "EXIT"
#define LIST_ALL "LS"
#define GET_CALL "GET"
#define PUT_CALL "PUT"
#define PRINT_DIR "PWD"
#define CHANGE_DIR "CD"
#define DELETE_FILE "DELETE"
#define CREATE_DIR "MKDIR"
#define DELETE_DIR "RMDIR"
#define STAT_CALL "STAT"

// calls made for FUSE ONLY
#define RENAME_CALL "REN"
#define FUSE_FLAG "-f"
#define TRUNCATE_CALL "TRUNC"
#define UTIMENS_CALL "TIME"
#define CHMOD_CALL "MOD"

typedef enum{
    STATUS_OK = 0,
    STATUS_ERROR = -1,
    STATUS_NOT_EMPTY = 1,
} Status;

typedef struct{
    Status status;
} Response;

typedef struct {
    Status status;
    uint64_t size;
    uint32_t mode;
    uint32_t nlink;
    struct timespec atime;
    struct timespec mtime;   
} FileStat;


void errNClose(const char *msg,int fd);
int createSocket();
int sendRecursively(int sock, const void *buffer, uint32_t length);
int send_msg (int fd , const void *buffer, uint32_t length);
int recvHelper(int fd, void *buffer, size_t length);
char *get_msg(int client_fd);
uint32_t getHeader(int client_fd);
int getFiles(int fd);
int get_fileData(int sock,char fileName[]);
off_t get_size(char *fileName);
char *getArgument(char *req);
int send_file(int fd, char *fileName);

#endif