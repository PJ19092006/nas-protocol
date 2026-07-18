#ifndef COMMON_H
#define COMMON_H
#include <stdio.h>
#include <stdint.h>

#define PORT 5000
#define BUFFER_SIZE 1024
#define err "err"

void errNClose(const char *msg,int fd);
void closeFd(int fd);
int createSocket();
// the new added func to common
int send_all(int sock, const void *buffer, uint32_t length);
int send_msg (int fd , const void *buffer, uint32_t length);
int recv_all(int fd, void *buffer, size_t length);
char *getMsg(int client_fd);
uint32_t getHeadder(int client_fd);

#endif