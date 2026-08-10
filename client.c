#include "common.h"

int listenCalls(char *req, int fd);
int connectServer(int sock);

char *currDir = NULL;

int main(){
    int sock = createSocket();
    if (sock == -1)errNClose("socket",sock);

    int connectRes = connectServer(sock);

    if (connectRes != -1){
        int bytes = send_msg(sock, PRINT_DIR, strlen(PRINT_DIR));

        Response res;
        recvHelper(sock, &res, sizeof(res));
        
        if(res.status != STATUS_OK){
            printf("Couldn't get working directory.\n");
            return -1;
        }
        currDir = get_msg(sock);

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
    char *fileName = getArgument(req);
    int bytes = 0;

    if(strcmp(LIST_ALL,req) == 0){
        bytes = getFiles(fd);

    }else if(strncmp(GET_CALL,req,3) == 0){
        Response res;
        recvHelper(fd, &res, sizeof(res));

        if(res.status == STATUS_OK){
            // get the file name
            printf("what name of the file you want: ");
            char newFileName[100];
            fgets(newFileName,100,stdin);
            newFileName[strcspn(newFileName, "\n")] = '\0'; 

            bytes = get_fileData(fd,newFileName);

        }else{
            printf("Server side error");
        }

    }else if(strncmp(PUT_CALL,req,3) == 0){ 
        printf("what name of the file you want: ");
        char newFileName[100];
        fgets(newFileName,100,stdin);
        newFileName[strcspn(newFileName, "\n")] = '\0';
        send_msg(fd,newFileName,strlen(newFileName));
        bytes = send_file(fd,fileName);
    }else if(strcmp(EXIT,req) == 0){
        printf("bye bye!");
        return -1;
    }else if(strcmp(PRINT_DIR,req) == 0){
        Response res;
        recvHelper(fd, &res, sizeof(res));
        if(res.status == STATUS_OK){
            free(currDir);
            currDir = get_msg(fd);
            if(currDir == NULL) return -1;
            printf("current directory is: %s\n",currDir);
        }else{
            printf("Something went wrong.\n");
        }
    }else if(strncmp(CHANGE_DIR,req,2)==0){
        Response res;
        recvHelper(fd, &res, sizeof(res));
        if(res.status == STATUS_OK){
            free(currDir);
            currDir = get_msg(fd);
            if(currDir == NULL) return -1;
            printf("directory changed: %s\n",currDir);
        }else{
            printf("Something went wrong.\n");
        }

    }else if(strncmp(DELETE_FILE,req,6)==0){
        Response res;
        recvHelper(fd, &res, sizeof(res));
        if(res.status == STATUS_OK){
            printf("File deleted successfully.\n");
        }else{
            printf("Failed to delete file.\n");
        }
    }else if(strncmp(CREATE_DIR,req,5)==0 ){
        Response res;
        recvHelper(fd, &res, sizeof(res));
        if(res.status == STATUS_OK){
            printf("New directory created SUCCESSFULLY.\n");
        }else{
            printf("FAILED to create new directory.\n");
        }
    }else if(strncmp(DELETE_DIR,req,5)==0){
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