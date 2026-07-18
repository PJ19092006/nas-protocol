#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // std lib
#include <sys/socket.h>  // socket lib
#include <stdio.h>  // std input
#include <stdlib.h>
#include <dirent.h> // to use dir
#include <string.h>
#include "common.h"

void analyzeCalls(char *req, int fd); 
void listAll(int fd);
void dirEnd(int fd);
int bindSocket(int sock);

int main(){
    // create socket
    int sock = createSocket();
    if (sock == -1) errNClose("socket",sock);

    // bind socket
    int bindRes = bindSocket(sock);
    if (bindRes == -1) errNClose("bind", sock);

    // listen for req
    int listenRes = listen(sock,5);
    if (listenRes == -1) errNClose("listen",sock);

    // accept the req
    int client_fd = accept(sock, NULL, NULL);
    if(client_fd == -1){
        errNClose("accpet",client_fd);
        closeFd(sock);
    }

    // extracting the headder of the protocol (res)
    char *buffer = getMsg(client_fd);
    if (strcmp(buffer,err) == 0){
        return -1;
    }
    
    printf("%s\n",buffer);
    analyzeCalls(buffer,client_fd);
    dirEnd(client_fd);

    free(buffer); 
    closeFd(sock);
    closeFd(client_fd);
    return 0;
}

void analyzeCalls(char *req,int fd){
    char listCall[] = "LIST";
    if(strcmp(listCall,req) == 0){
        listAll(fd);
    }
}

void listAll(int fd){
    DIR *dir = opendir(".");
    struct dirent *entry;

	while ((entry = readdir(dir)) != NULL) {
        char *file = entry->d_name;
        int bytes = send_msg(fd,file,strlen(file));
        if(bytes == -1) return;
	}

    closedir(dir);
}

void dirEnd(int fd){
    char *msg = "end";
    send_msg(fd,msg,strlen(msg));
}

int bindSocket(int sock){
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    int bindRes = bind(
        sock,
        (struct sockaddr *)&address,
        sizeof(address)
    );

    return bindRes;
}