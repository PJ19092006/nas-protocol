#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h> 
#include <string.h>


void errNClose(const char *msg,int arrFDS[]){
    closeFD(arrFDS);
    perror(msg);
    exit(EXIT_FAILURE);
}

void closeFD(int arrFDS[]){
    for(int i = 0; i<2 || arrFDS[i] == 0;i++){
            close(arrFDS[i]);
    }
}
int createSocket(void){
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == -1) {
        perror("socket");
        return -1;
    }
    return sock;  
}

int sendMessage(int fd, const char *msg){
    int bytes = write(fd,msg,strlen(msg));
    return bytes;
}
int receiveMessage(int fd, char *buffer, size_t size){
    int bytes = read(fd,buffer,size);
    return bytes;
}

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