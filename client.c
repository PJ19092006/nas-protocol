#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // std lib
#include <sys/socket.h>  // socket lib
#include <stdio.h>  // std input
#include <string.h>
#include <stdlib.h>
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

    // new code
    char msg[] = "LIST";
    int bytes = send_msg(sock, msg, strlen(msg));
    if (bytes == -1)errNClose("read",fd);
    // end

    while(1 == 1){

    uint32_t netLength ;
    recv_all(sock,&netLength,sizeof(netLength));
    uint32_t length = ntohl(netLength);

    if(length > 1000) return -1; // overlimit memory call

    // allocating the memory reviced from server
    char *buffer;
    buffer = (char *) malloc(length + 1);

    if(buffer == NULL) return -1; // if the malloc fails
    int bytes = recv_all(sock,buffer,length);
    buffer[bytes] = '\0';
    char end[] = "end";
    if (strcmp(end,buffer)) return 0;
    
    printf("%s\n",buffer);
    }

    closeFD(fd);
    return 0;
}