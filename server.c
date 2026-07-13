#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // std lib
#include <sys/socket.h>  // socket lib
#include <stdio.h>  // std input
#include <stdlib.h>
#include <dirent.h> // to use dir
#include <string.h>
#include "common.h"

void analyzeCalls(char req[], int fd); 
void listAll(int fd);
void dirEnd(int fd);

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


    // extracting the headder of the protocol
    uint32_t netLength ;
    recv_all(client_fd,&netLength,sizeof(netLength));
    uint32_t length = ntohl(netLength);

    if(length > 1000) return -1; // overlimit memory call

    // allocating the memory reviced from client 
    char *buffer;
    buffer = (char *) malloc(length + 1);

    if(buffer == NULL) return -1; // if the malloc fails
    int bytes = recv_all(client_fd,buffer,length);

    if(bytes == -1){
        errNClose("read",fds);
        return -1;
    }

    buffer[bytes] = '\0';

    printf("%s\n",buffer);

    analyzeCalls(buffer,client_fd);
    dirEnd(client_fd);

    free(buffer); 
    closeFD(fds);
    return 0;
}

void analyzeCalls(char req[],int fd){
    char listCall[] = "LIST";
    if(strcmp(listCall,req)){
        listAll(fd); // to be defined
    }
}

void listAll(int fd){
    DIR *dir = opendir(".");
    struct dirent *entry;

	while ((entry = readdir(dir)) != NULL) {
        char *file = entry->d_name;
        int bytes = send_all(fd,file,sizeof(file));
        if(bytes == -1) return;
	}

    closedir(dir);
	// closedir(dir); // might not cause we still need res from client in next phase
}

void dirEnd(int fd){
    char *msg = "end";
    send_all(fd,msg,sizeof(msg));
}