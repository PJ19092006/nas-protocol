#include "common.h"

int listenCalls(char *req, int fd);
int connectServer(int sock);

int main(){
    int sock = createSocket();
    if (sock == -1)errNClose("socket",sock);

    int connectRes = connectServer(sock);

    if (connectRes != -1){
        while (1){
            char msg[BUFFER_SIZE];
            fgets(msg,BUFFER_SIZE,stdin);
            msg[strcspn(msg, "\n")] = '\0';
            
            int bytes = send_msg(sock, msg, strlen(msg));
            if (bytes == -1)errNClose("read",sock);
            int res = listenCalls(msg,sock);
            if(res == -1) return -1;
        }
        
    }else{
        errNClose("connect", sock);
    }

    close(sock);
    return 0;
}

int listenCalls(char *req, int fd){
    char exitCall[] = "EXIT";
    char listCall[] = "LIST";
    char opendDirCall[] = "GET";
    char addDirCall[] = "PUT";
    char *fileName = getArgument(req);
    int bytes = -1;

    if(strcmp(listCall,req) == 0){
        bytes = getFiles(fd);
        if(bytes == -1) return -1;
    }else if(strncmp(opendDirCall,req,3) == 0){ // GET call 
        printf("what name of the file you want: ");
        char newFileName[100];
        fgets(newFileName,100,stdin);
        newFileName[strcspn(newFileName, "\n")] = '\0'; 
        bytes = get_fileData(fd,newFileName);
        if(bytes == -1) errNClose("transfer",fd);
    }else if(strncmp(addDirCall,req,3) == 0){ // PUT call
        bytes = read_func(fd,fileName);
    }else if(strcmp(req,exitCall) == 0){
        return -1;
    }else if(strcmp(req, "PWD") == 0){
        uint32_t size;
        char *path = get_msg(fd, &size);
        if(path == NULL) return -1;
        printf("%s\n", path);
        free(path);
}

    return bytes;
}

int connectServer(int sock){
    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); // addr of client

    int connectRes = connect(sock,(struct sockaddr *)&server,sizeof(server));

    return connectRes;
}