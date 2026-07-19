#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // std lib
#include <sys/socket.h>  // socket lib
#include <stdio.h>  // std input
#include <string.h>
#include <stdlib.h>
#include "common.h"

int connectServer(int sock);

int main(){
    // create sock
    int sock = createSocket();
    if (sock == -1)errNClose("socket",sock);

    // connect to server
    int connectRes = connectServer(sock);
    if (connectRes == -1) errNClose("connect", sock);

    // send the msg to server
    char msg[] = "LIST";
    int bytes = send_msg(sock, msg, strlen(msg));
    if (bytes == -1)errNClose("read",sock);

    char end[] = "end"; // declaring to check if file dir ended (this is for now only, to be changed)

    // listning for dir (response)
    int x = getFiles(sock);
    if (x == -1) errNClose("read",sock);

    closeFd(sock);
    return 0;
}


int connectServer(int sock){
    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(5000);
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); // addr of client

    int connectRes = connect(sock,(struct sockaddr *)&server,sizeof(server));

    return connectRes;
}