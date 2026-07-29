#include "common.h"

int listenCalls(char *req, int fd);
int connectServer(int sock);

char *currDir = NULL;

int main(){
    int sock = createSocket();
    if (sock == -1)errNClose("socket",sock);

    int connectRes = connectServer(sock);
    char pwdCall[] = "PWD";

    if (connectRes != -1){
        int bytes = send_msg(sock, pwdCall, strlen(pwdCall));

        Response res;
        recvHelper(sock, &res, sizeof(res));
        
        if(res.status != STATUS_OK){
            printf("Couldn't get working directory.\n");
            return -1;
        }
        uint32_t length;
        currDir = get_msg(sock, &length);

        while (1){
            printf("%s> ",currDir);
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
    char currDirCall[] = "PWD";
    char changeDirCall[] = "CD";

    char deleteDirCall[] = "RMDIR";
    char deleteCall[] = "DELETE";
    char newDirCall[] = "MKDIR";

    char *fileName = getArgument(req);
    int bytes = 0;

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
        printf("bye bye!");
        return -1;
    }else if(strcmp(req, currDirCall) == 0){
        Response res;
        recvHelper(fd, &res, sizeof(res));
        if(res.status == STATUS_OK){
            uint32_t size;
            free(currDir);
            currDir = get_msg(fd, &size);
            if(currDir == NULL) return -1;
            printf("current directory is: %s\n",currDir);
        }else{
            printf("Something went wrong.\n");
        }
    }else if(strncmp(req,changeDirCall,2)==0){
        Response res;
        recvHelper(fd, &res, sizeof(res));
        if(res.status == STATUS_OK){
            uint32_t size;
            free(currDir);
            currDir = get_msg(fd, &size);
            if(currDir == NULL) return -1;
            printf("directory changed: %s\n",currDir);
        }else{
            printf("Something went wrong.\n");
        }

    }else if(strncmp(req,deleteCall,6)==0){
        Response res;
        recvHelper(fd, &res, sizeof(res));
        if(res.status == STATUS_OK){
            printf("File deleted successfully.\n");
        }else{
            printf("Failed to delete file.\n");
        }
    }else if(strncmp(req,newDirCall,5)==0 ){
        Response res;
        recvHelper(fd, &res, sizeof(res));
        if(res.status == STATUS_OK){
            printf("New directory created SUCCESSFULLY.\n");
        }else{
            printf("FAILED to create new directory.\n");
        }
    }else if(strncmp(req,deleteDirCall,5)==0){
        Response res;
        recvHelper(fd, &res, sizeof(res));
        if(res.status == STATUS_OK){
            printf("Directory deleted successfully.\n");
        }else{
            printf("Failed to delete directory.\n");
        }
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