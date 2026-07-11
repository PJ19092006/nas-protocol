#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
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