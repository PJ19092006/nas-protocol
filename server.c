#include "common.h"
#include <signal.h>
#include <sys/wait.h>

int establishConnection();
int analyzeCalls(char *req, int fd); 
int listAll(int fd);
int bindSocket(int sock);
int delete_func(char *fileName,int fd);
int change_dir(const char *dirname,int fd);
int create_dir(char *fileName,int fd);
int remove_dir(char *dirName,int fd);
int getWorking_dir(int fd);
void sendStatus(int bytes, int fd);
void handle_child();
int getStat(int fd, char *fileName);

int main(){
    signal(SIGCHLD, handle_child);
    int sock = establishConnection();
    if (sock == -1)errNClose("socket", sock);

    while (1){
        int clientFd = accept(sock, NULL, NULL);
        if (clientFd == -1){
            perror("accept");
            continue;
        }

        pid_t pid = fork();

        if(pid < 0){
            perror("fork");
            close(clientFd);
            continue;
        }

        if(pid == 0){ 
            close(sock);
            while (1){
                char *buffer = get_msg(clientFd);
    
                if (buffer == NULL)
                    break;
    
                printf("%s\n", buffer);
    
                int res = analyzeCalls(buffer, clientFd);
    
                free(buffer);
    
                if (res == -1)break;
            }
    
            close(clientFd);
            exit(0);
        }
        close(clientFd);
    }

    close(sock);
    return 0;
}

int analyzeCalls(char *req,int fd){
    char *fileName = getArgument(req);
    int bytes=-1;

    if(strcmp(LIST_ALL,req) == 0){
        bytes = listAll(fd);
    }else if(strncmp(GET_CALL,req,3) == 0){
        if(fileName == NULL)return -1;
        bytes = send_file(fd,fileName);
    }else if(strncmp(PUT_CALL,req,3) == 0){
        char *newFileName = get_msg(fd);
        bytes = get_fileData(fd,newFileName);
        free(newFileName);
    }else if(strncmp(DELETE_FILE,req,6) == 0){
        if(fileName == NULL)return -1;
        bytes = delete_func(fileName,fd);
    }else if(strncmp(CREATE_DIR,req,5) == 0){
        if(fileName == NULL)return -1;
        bytes = create_dir(fileName,fd);
    }else if(strcmp(EXIT,req) == 0){
        return -1;
    }else if(strcmp(PRINT_DIR,req) == 0){
        bytes = getWorking_dir(fd); 
    }else if(strncmp(CHANGE_DIR,req,2)==0){
        if(fileName == NULL)return -1;
        bytes = change_dir(fileName,fd);
    }else if(strncmp(DELETE_DIR,req,5) == 0){
        if(fileName == NULL)return -1;
        bytes = remove_dir(fileName,fd);
    }else if(strncmp(STAT_CALL,req,4)==0){
        if(fileName == NULL) return -1;
        bytes = getStat(fd,fileName);
    }

    if(bytes == -1) return -1;
    return bytes;
}

int getStat(int fd, char *fileName){
    struct stat info;
    Response res;

    if (stat(fileName, &info) != 0) {
        res.status = STATUS_ERROR;

        if (sendRecursively(fd, &res, sizeof(res)) == -1)
            return -1;

        return 0;
    }

    res.status = STATUS_OK;

    if (sendRecursively(fd, &res, sizeof(res)) == -1)
        return -1;

    FileStat file;

    file.size = info.st_size;
    file.mode = info.st_mode;
    file.nlink = info.st_nlink;

    return sendRecursively(fd, &file, sizeof(file));
}

// the new functions are being added up here
int create_dir(char *dirName,int fd){
    int bytes = mkdir(dirName,0755);
    sendStatus(bytes,fd);
    return bytes;
}

int remove_dir(char *dirName,int fd){
    int bytes = rmdir(dirName);
    sendStatus(bytes,fd);
    return bytes;
}

int getWorking_dir(int fd){
    Response res;
    char *dirName = getcwd(NULL, 0);
    
    if(dirName == NULL){
        res.status = STATUS_ERROR;
        sendRecursively(fd, &res, sizeof(res));
        return -1;
    }

    res.status = STATUS_OK;
    sendRecursively(fd, &res, sizeof(res));

    int bytes = send_msg(fd, dirName, strlen(dirName));
    free(dirName);

    return bytes;
}

int change_dir(const char *dirName, int fd){
    Response res;
    int bytes = chdir(dirName);
    char *path = getcwd(NULL,0);

    if(bytes == -1 || path == NULL){
        res.status = STATUS_ERROR;
        sendRecursively(fd, &res, sizeof(res));
        return -1;
    }

    res.status = STATUS_OK;
    sendRecursively(fd, &res, sizeof(res));

    send_msg(fd,path,strlen(path));
    free(path);

    return 0;
}

int delete_func(char *fileName,int fd){
    int bytes = remove(fileName);
    sendStatus(bytes,fd);
    return bytes;
}

void sendStatus(int bytes, int fd){
    Response res;
    if(bytes != -1)res.status = STATUS_OK;
    else res.status = STATUS_ERROR;
    sendRecursively(fd, &res, sizeof(res));
}

int listAll(int fd){

    DIR *dir = opendir(".");
    if(dir == NULL) return -1;
    struct dirent *entry; // pointer pointing to dirent structure
    int count = 0;

	while ((entry = readdir(dir)) != NULL) {
        count ++;
	}

    uint32_t totalFiles = htonl(count);
    sendRecursively(fd,&totalFiles,sizeof(totalFiles));

    closedir(dir);

    dir = opendir(".");
    if(dir == NULL) return -1;
    int totalBytes = 0;
    for(int i = 0; i<count; i++){
        entry = readdir(dir);
        char *file = entry->d_name;
        int bytes = send_msg(fd,file,strlen(file));
        if(bytes == -1){
            closedir(dir);
            return -1;
        }
        totalBytes += bytes;
    }

    closedir(dir);
    return totalBytes;
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

// removed the sig argument from this function 
void handle_child(){
    while (waitpid(-1, NULL, WNOHANG) > 0);
}