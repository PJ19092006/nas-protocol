#include "common.h"

void listenCalls(char *req, int fd);
int connectServer(int sock);

int main(){
    // create sock
    int sock = createSocket();
    if (sock == -1)errNClose("socket",sock);

    // connect to server
    int connectRes = connectServer(sock);
    if (connectRes == -1) errNClose("connect", sock);

    // taking msg from client
    char msg[100];
    fgets(msg,100,stdin);
    msg[strcspn(msg, "\n")] = '\0';

    int bytes = send_msg(sock, msg, strlen(msg));
    if (bytes == -1)errNClose("read",sock);
    listenCalls(msg,sock);

    close(sock);
    return 0;
}

void listenCalls(char *req, int fd){
    char listCall[] = "LIST";
    char opendDirCall[] = "GET";
    char addDirCall[] = "PUT";
    char *fileName = req + 4;

    if(strcmp(listCall,req) == 0){
        int totalBytes = getFiles(fd);
        if(totalBytes == -1) return;
    }else if(strncmp(opendDirCall,req,3) == 0){ // GET call 
        printf("what name of the file you want: ");
        char fileName[100];
        fgets(fileName,100,stdin);
        fileName[strcspn(fileName, "\n")] = '\0'; 
        int fileBytesRecv = get_fileData(fd,fileName);
        if(fileBytesRecv == -1) errNClose("transfer",fd);
    }else if(strncmp(addDirCall,req,3) == 0){ // PUT call
        int n = read_func(fd,fileName);
    }
}

int connectServer(int sock){
    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(5000);
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); // addr of client

    int connectRes = connect(sock,(struct sockaddr *)&server,sizeof(server));

    return connectRes;
}