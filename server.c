#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // std lib
#include <sys/socket.h>  // socket lib
#include <stdio.h>  // std input
#include "common.h"

int main(){

    int sock = createSocket();
    int fds[2] = {sock,0};
    if (sock == -1) errNClose("socket",fds);

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    int bindres = bind(
        sock,
        (struct sockaddr *)&address,
        sizeof(address)
    );

    if(bindres == -1) errNClose("bind",fds);

    int listenRes = listen(sock,5);
    if (listenRes == -1) errNClose("listen",fds);

    int client_fd = accept(sock, NULL, NULL);

    fds[1]= client_fd;
    if(client_fd == -1) errNClose("accept",fds);

    char buffer[BUFFER_SIZE];
    int bytes = receiveMessage(client_fd,buffer,sizeof(buffer));

    if(bytes == -1) errNClose("read",fds);
    buffer[bytes]='\0';
    printf("client: %s\n",buffer);

    int writeRes = sendMessage(client_fd,buffer);
    if(writeRes == -1 ) errNClose("write",fds);

    closeFD(fds);

    return 0;
}