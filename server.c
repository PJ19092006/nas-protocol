#include "common.h"
#include <signal.h>
#include <sys/wait.h>



int establishConnection();
int analyzeCalls(char *req, int fd); 
int listAll(int fd, char *dirName);
int bindSocket(int sock);
int delete_func(char *fileName,int fd);
int change_dir(const char *dirname,int fd);
int create_dir(char *fileName,int fd);
int remove_dir(char *dirName,int fd);
int getWorking_dir(int fd);
void sendStatus(int bytes, int fd);
void handle_child(int sig);
int getStat(int fd, char *fileName);
int hasFlag(char *req);
int parseGetChunk(char *req,char *fileName,off_t *offset,size_t *size);
int getFileChunk(int fd,char *fileName,off_t offset,size_t requestedSize);
int handle_truncate(int fd, char *fileName);
int handle_rename(int fd);
int handle_utimens(int fd, char *fileName);
int handle_chmod(int fd, char *fileName);

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

    if(strncmp(LIST_ALL,req,2) == 0){
        bytes = listAll(fd, fileName);
    }else if(strncmp(GET_CALL, req, 3) == 0){
    if (hasFlag(req)) {
        char fileName[256];
        off_t offset;
        size_t size;
        if (parseGetChunk(req, fileName, &offset, &size) == -1) return -1;

        bytes = getFileChunk(fd,fileName,offset,size);
    } else {
        char *fileName = getArgument(req);
        if (fileName == NULL)return -1;
        bytes = send_file(fd, fileName);
    }
    }else if(strncmp(PUT_CALL,req,3) == 0){
        // char *newFileName = get_msg(fd);
        bytes = get_fileData(fd,fileName);
        // free(newFileName);
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
    }else if(strncmp(TRUNCATE_CALL,req,5)==0){
        if(fileName == NULL) return -1;
        bytes = handle_truncate(fd,fileName);
    }else if(strncmp(RENAME_CALL, req, 3) == 0){
        bytes = handle_rename(fd);
    }else if(strncmp(UTIMENS_CALL, req, 4) == 0){
        if (fileName == NULL) return -1;
        bytes = handle_utimens(fd, fileName);
    } else if(strncmp(CHMOD_CALL, req, 3) == 0){
        if (fileName == NULL) return -1;
        bytes = handle_chmod(fd, fileName);
    }

    if(bytes == -1) return -1;
    return bytes;
}

int hasFlag(char *req){
    return strstr(req, FUSE_FLAG) != NULL;
}

int parseGetChunk(char *req,char *fileName,off_t *offset,size_t *size){
    char flag[10];

    int result = sscanf(req,"GET %255s %9s %ld %zu",fileName,flag,offset,size);

    if (result != 4)
        return -1;

    if (strcmp(flag, FUSE_FLAG) != 0)
        return -1;

    return 0;
}

int getFileChunk(int fd,char *fileName,off_t offset,size_t requestedSize){

    Response res;
    int fileFd = open(fileName, O_RDONLY);
    if (fileFd == -1) {
        res.status = STATUS_ERROR;
        sendRecursively(fd, &res, sizeof(res));
        return 0;
    }

    off_t fileSize = get_size(fileName);

    if (fileSize < 0) {
        close(fileFd);

        res.status = STATUS_ERROR;
        sendRecursively(fd, &res, sizeof(res));
        return 0;
    }

    // If FUSE asks beyond EOF.
    if (offset >= fileSize) {

        res.status = STATUS_OK;
        sendRecursively(fd, &res, sizeof(res));

        uint64_t zero = 0;
        uint64_t networkZero = htobe64(zero);

        sendRecursively(fd,&networkZero,sizeof(networkZero));
        close(fileFd);
        return 0;
    }

    // bytes requested
    size_t bytesToSend = requestedSize;

    if (offset + bytesToSend > fileSize) bytesToSend = fileSize - offset;

    // changing the offset (to requested)
    if (lseek(fileFd, offset, SEEK_SET) == -1) {
        close(fileFd);
        res.status = STATUS_ERROR;
        sendRecursively(fd, &res, sizeof(res));
        return 0;
    }

    res.status = STATUS_OK;

    // sending the status
    if (sendRecursively(fd,&res,sizeof(res)) == -1) {
        close(fileFd);
        return -1;
    }

    // sending bytes size
    uint64_t networkSize = htobe64(bytesToSend);

    if (sendRecursively(fd,&networkSize,sizeof(networkSize)) == -1) {
        close(fileFd);
        return -1;
    }

    char chunk[4096];
    size_t remaining = bytesToSend;

    while (remaining > 0) {

        size_t toRead = remaining > sizeof(chunk)? sizeof(chunk): remaining;

        ssize_t bytesRead = read(fileFd, chunk, toRead);

        if (bytesRead <= 0) {
            close(fileFd);
            return -1;
        }

        if (sendRecursively(fd,chunk,bytesRead) == -1) {
            close(fileFd);
            return -1;
        }

        remaining -= bytesRead;
    }

    close(fileFd);
    return bytesToSend;
}

int getStat(int fd, char *fileName){
    struct stat info;
    Response res;

    if (stat(fileName, &info) != 0) {
        res.status = STATUS_ERROR;
        sendRecursively(fd, &res, sizeof(res));
        return 0;
    }

    res.status = STATUS_OK;
    sendRecursively(fd, &res, sizeof(res));

    FileStat file;
    file.size = info.st_size;
    file.mode = info.st_mode;
    file.nlink = info.st_nlink;
    file.atime = info.st_atim;
    file.mtime = info.st_mtim;

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

int listAll(int fd, char *dirName){
    printf("%s\n",dirName);
    DIR *dir = opendir(dirName);

    if(dir == NULL) {
        uint32_t zero = 0;
        sendRecursively(fd, &zero, sizeof(zero)); // Tell client: 0 files
        return 0; // Keep the server alive!
    }

    struct dirent *entry; // pointer pointing to dirent structure
    int count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||strcmp(entry->d_name, "..") == 0) continue;
        count++;
    }

    uint32_t totalFiles = htonl(count);
    sendRecursively(fd,&totalFiles,sizeof(totalFiles));
    closedir(dir);

    dir = opendir(dirName);
    if(dir == NULL) return -1;
    int totalBytes = 0;

    //  to be formmated here
    for (int i = 0; i < count; ) {

    entry = readdir(dir);

    if (entry == NULL)
        break;

    if (strcmp(entry->d_name, ".") == 0 ||
        strcmp(entry->d_name, "..") == 0) {
        continue;
    }

    char *file = entry->d_name;

    int bytes = send_msg(fd, file, strlen(file));

    if (bytes == -1) {
        closedir(dir);
        return -1;
    }

    totalBytes += bytes;
    i++;
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

int handle_truncate(int fd, char *fileName){
    Response res;
    uint64_t networkSize;

    if (recvHelper(fd, &networkSize, sizeof(networkSize)) == -1) return -1;
    uint64_t targetSize = be64toh(networkSize);

    int result = truncate(fileName, (off_t)targetSize);

    if (result == -1) {
        res.status = STATUS_ERROR;
        sendRecursively(fd, &res, sizeof(res));
        return -1;
    }

    res.status = STATUS_OK;
    sendRecursively(fd, &res, sizeof(res));
    return 0;
}

int handle_rename(int fd){
    Response res;
    

    char *oldName = get_msg(fd);
    if (oldName == NULL) return -1;

    char *newName = get_msg(fd);
    if (newName == NULL) {
        free(oldName);
        return -1;
    }

    int result = rename(oldName, newName);

    free(oldName);
    free(newName);

    if (result == -1) {
        res.status = STATUS_ERROR;
        sendRecursively(fd, &res, sizeof(res));
        return -1;
    }

    res.status = STATUS_OK;
    sendRecursively(fd, &res, sizeof(res));
    return 0;
}

int handle_utimens(int fd, char *fileName){
    Response res;
    struct timespec tv[2];

    if (recvHelper(fd, tv, sizeof(tv)) == -1) return -1;

    // AT_FDCWD means relative to current working directory
    int result = utimensat(AT_FDCWD, fileName, tv, 0);

    if (result == -1) {
        res.status = STATUS_ERROR;
        sendRecursively(fd, &res, sizeof(res));
        return -1;
    }

    res.status = STATUS_OK;
    sendRecursively(fd, &res, sizeof(res));
    return 0;
}

int handle_chmod(int fd, char *fileName){
    Response res;
    uint32_t networkMode;

    if (recvHelper(fd, &networkMode, sizeof(networkMode)) == -1) return -1;
    mode_t newMode = (mode_t)ntohl(networkMode);

    int result = chmod(fileName, newMode);

    if (result == -1) {
        res.status = STATUS_ERROR;
        sendRecursively(fd, &res, sizeof(res));
        return -1;
    }

    res.status = STATUS_OK;
    sendRecursively(fd, &res, sizeof(res));
    return 0;
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

void handle_child(int sig){
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}