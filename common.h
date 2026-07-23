#ifndef COMMON_H
#define COMMON_H
#include <stdio.h>
#include <stdint.h>

#define PORT 5000
#define BUFFER_SIZE 1024

void errNClose(const char *msg,int fd);
void closeFd(int fd);
int createSocket();
// the new added func to common
int sendHelper(int sock, const void *buffer, uint32_t length);
int send_msg (int fd , const void *buffer, uint32_t length);
int recvHelper(int fd, void *buffer, size_t length);
char *getMsg(int client_fd);
uint32_t getHeadder(int client_fd);
int getFiles(int fd);

#endif