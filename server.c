#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>  
#include <stdlib.h>
#include <dirent.h> 
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "common.h"

int establishConnection();
void analyzeCalls(char *req, int fd); 
void listAll(int fd);
int bindSocket(int sock);
int read_func(char *req, int fd);
size_t stat_func(char *fileName);

int main(){
    int sock = establishConnection();
    if(sock == -1) errNClose("sock",sock);

    int client_fd = accept(sock, NULL, NULL);
    if(client_fd == -1){
        errNClose("accpet",client_fd);
        closeFd(sock);
    }

    // extracting the headder of the protocol (res)
    char *buffer = getMsg(client_fd);
    if (buffer == NULL) return -1;
    
    printf("%s\n",buffer);
    analyzeCalls(buffer,client_fd);

    free(buffer); 
    closeFd(sock);
    closeFd(client_fd);
    return 0;
}

void analyzeCalls(char *req,int fd){
    char listCall[] = "LIST";
    char opendDirCall[] = "GET";

    if(strcmp(listCall,req) == 0){
        listAll(fd);
    }else if(strncmp(opendDirCall,req,4) == 0){
        int n = read_func(req,fd);
        if(n==-1) return;
    }
}

size_t stat_func(char *fileName){
    struct stat info; // Stat structure using it as info
    size_t size;
	int n = stat(fileName,&info);
	if(n == 0){
        size = info.st_size;
		if (S_ISDIR(info.st_mode)){
			printf("for the directory named :%s, stat is %ld\n", fileName,size);
		}else if (S_ISREG(info.st_mode)){
            size = info.st_size;
			printf("for the file named :%s, stat is %ld\n", fileName,info.st_size);
		}
        return size;
	}

    return -1;
}

int read_func(char *req, int fd){
    char *fileName = req + 4;
    printf("%s\n", fileName);
    
    size_t size = stat_func(fileName);
    if(size == -1) return -1;

    char *ch = malloc(size);
	int fileFd = open(fileName, O_RDONLY);
    if (fileFd == -1) return -1;
    
    int n = read(fileFd,ch,size);

    if (n<=0) return -1;
    int bytes = send_msg(fd,ch,n);

    close(fileFd);
    free(ch);
    return bytes;
}

void listAll(int fd){
    DIR *dir = opendir("."); // opening the dir and using pointer to point at it
    struct dirent *entry; // pointer pointing to dirent structure
    int count = 0;

	while ((entry = readdir(dir)) != NULL) {
        count ++;
	}

    uint32_t toalFiles = htonl(count);
    sendHelper(fd,&toalFiles,sizeof(toalFiles));

    closedir(dir);
    dir = opendir(".");

    for(int i = 0; i<count; i++){
        entry = readdir(dir);
        char *file = entry->d_name;
        int bytes = send_msg(fd,file,strlen(file));
        if(bytes == -1) return;
    }

    closedir(dir);
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

int establishConnection(){
    // create socket
    int sock = createSocket();
    if (sock == -1) errNClose("socket",sock);

    // bind socket
    int bindRes = bindSocket(sock);
    if (bindRes == -1) errNClose("bind", sock);

    // listen for req
    int listenRes = listen(sock,5);
    if (listenRes == -1) errNClose("listen",sock);
    return sock;
}