#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>  
#include <stdlib.h>
#include <dirent.h> 
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h> 

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

typedef enum{
    STATUS_OK = 0,
    STATUS_ERROR
} Status;

typedef struct{
    Status status;
} Response;

typedef struct {
    Status status;
    uint64_t size;
    uint32_t mode;
    uint32_t nlink;
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
size_t get_size(char *fileName);
char *getArgument(char *req);
int send_file(int fd, char *fileName);

#endif