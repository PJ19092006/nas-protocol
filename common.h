#ifndef COMMON_H
#define COMMON_H
#include <stdio.h>

#define PORT 5000
#define BUFFER_SIZE 1024

void errNClose(const char *msg,int arrFDS[]);
void closeFD(int arrFDS[]);

int createSocket(void);
int sendMessage(int fd, const char *msg);
int receiveMessage(int fd, char *buffer, size_t size);

#endif