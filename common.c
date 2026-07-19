#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h> 
#include <string.h>


void errNClose(const char *msg,int fd){
    closeFd(fd);
    perror(msg);
    exit(EXIT_FAILURE);
}

void closeFd(int fd){
    close(fd);
}

int createSocket(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == -1) {
        perror("socket");
        return -1;
    }
    return sock;  
}

// just becuase the TCP will not transfer the whole file at once (so a loop to keep track of that)
int send_all(int sock, const void *buffer, uint32_t length){
    const char *ptr = buffer;
    size_t remaining = length;

    while(remaining > 0){
        size_t sentBytes = write(sock, ptr,remaining);
        if (sentBytes <= 0) return -1;
        remaining -= sentBytes;
        ptr += sentBytes;
    }

    return length;
}

int send_msg (int fd , const void *buffer, uint32_t length){
    uint32_t lenghtMsg = htonl(length);

    // sending just the length (headder)
    int lengthBytes = send_all(fd,&lenghtMsg,sizeof(lenghtMsg));
    if (lengthBytes == -1) return -1;

    // sending the msg (body)
    int msgBytes = send_all(fd, buffer, length);
    if (msgBytes == -1) return -1;

    return lengthBytes + msgBytes;
}

char *getMsg(int fd){
    // extracting headder (length)
    uint32_t headder = getHeadder(fd);
    if(headder > 1000) return NULL; // overlimit memory call

    // alocating memory
    char *buffer;
    buffer = (char *) malloc(headder + 1);
    if(buffer == NULL) return NULL;

    // recive the msg
    int bytes = recv_all(fd,buffer,headder);

    buffer[bytes] = '\0';

    return buffer;
}

int getFiles(int fd){
    uint32_t totalFiles = getHeadder(fd);

    while(totalFiles>0){
        char *store = getMsg(fd);
        if(store == NULL) return -1;

        printf("%s\n",store);
        totalFiles--;
        free(store);
    }
}

int recv_all(int fd, void *buffer, size_t length){
    char *ptr = buffer;
    ssize_t remaining = length;
    int bytes = 0;

    while(remaining > 0){
        ssize_t recivedBytes = read(fd, ptr,remaining);
        if (recivedBytes <= 0) return -1;
        bytes+= recivedBytes;
        remaining -= recivedBytes;
        ptr += recivedBytes;
    }

    return bytes;
}

uint32_t getHeadder(int client_fd){
    uint32_t netLength ;
    int res = recv_all(client_fd,&netLength,sizeof(netLength));
    if (res == -1) return UINT32_MAX;
    uint32_t length = ntohl(netLength);

    return length;
}