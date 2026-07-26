#include "common.h"

int establishConnection();
void analyzeCalls(char *req, int fd); 
void listAll(int fd);
int bindSocket(int sock);

int main(){
    int sock = establishConnection();
    if(sock == -1) errNClose("sock",sock);

    int clientFd = accept(sock, NULL, NULL);
    if(clientFd == -1){
        errNClose("accpet",clientFd);
        close(sock);
    }

    // getting the request
    uint32_t size;
    char *buffer = get_msg(clientFd,&size);
    if (buffer == NULL) return -1;
    printf("%s\n",buffer);

    // passing the request further
    analyzeCalls(buffer,clientFd);
    

    free(buffer); 
    close(sock);
    close(clientFd);
    return 0;
}

void analyzeCalls(char *req,int fd){
    char listCall[] = "LIST";
    char opendDirCall[] = "GET";
    char addDirCall[] = "PUT";
    char *fileName = req + 4;

    if(strcmp(listCall,req) == 0){
        listAll(fd);
    }else if(strncmp(opendDirCall,req,3) == 0){
        int n = read_func(fd,fileName);
        if(n==-1) return;
    }else if(strncmp(addDirCall,req,3) == 0){
        printf("what name of the file you want: ");
        char newFileName[] = "newFile.jpeg" ;
        int n = get_fileData(fd,newFileName);
    }
}

void listAll(int fd){
    DIR *dir = opendir("."); // opening the dir and using pointer to point at it
    struct dirent *entry; // pointer pointing to dirent structure
    int count = 0;

	while ((entry = readdir(dir)) != NULL) {
        count ++;
	}

    uint32_t toalFiles = htonl(count);
    sendRecursively(fd,&toalFiles,sizeof(toalFiles));

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