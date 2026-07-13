#ifndef COMMON_H
#define COMMON_H
#include <stdio.h>
#include <stdint.h>

#define PORT 5000
#define BUFFER_SIZE 1024

void errNClose(const char *msg,int arrFDS[]);
void closeFD(int arrFDS[]);

int createSocket(void);
int sendMessage(int fd, const char *msg);
int receiveMessage(int fd, char *buffer, size_t size);
// the new added func to common
int send_all(int sock, const void *buffer, uint32_t length);
int send_msg (int fd , const void *buffer, uint32_t length);
int recv_all(int fd, void *buffer, size_t length);


#endif