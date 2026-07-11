#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // std lib
#include <sys/socket.h>  // socket lib
#include <stdio.h>  // std input
#include <string.h>
#include "common.h"

int main(){

    int sock = createSocket();
    int fd[1] = {sock}; 
    if (sock == -1)errNClose("socket",fd);

    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(5000);
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); // addr of client

    int connectRes = connect(sock,(struct sockaddr *)&server,sizeof(server));

    if (connectRes == -1) errNClose("connect",fd);

    char msg[] = "yohoo";
    // int n = sendMessage(sock,msg);

    send_all(sock,msg,sizeof(msg));

    char buffer[BUFFER_SIZE];
    int bytes = receiveMessage(sock,buffer,sizeof(buffer)-1);

    if (bytes == -1)errNClose("read",fd);

    buffer[bytes] = '\0';
    printf("server: %s\n",buffer);
    closeFD(fd);
    return 0;
}

int send_all(int sock, const void *buffer, size_t length){
    const char *ptr = buffer;
    int remaining = length;

    while(remaining > 0){
        size_t sentBytes = write(sock, ptr,remaining);
        if (sentBytes <= 0) return -1;
        remaining -= sentBytes;
        ptr += sentBytes;
    }

    return length;
}