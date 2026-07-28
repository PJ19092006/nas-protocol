#include "common.h"

int establishConnection();
int analyzeCalls(char *req, int fd); 
int listAll(int fd);
int bindSocket(int sock);
int delete_func(char *fileName);
int change_dir(const char *dirname);
int create_dir(char *fileName);
int remove_dir(char *dirName);
int getWorking_dir(int fd);

int main(){
    int sock = establishConnection();
    if (sock == -1)errNClose("socket", sock);

    while (1){
        int clientFd = accept(sock, NULL, NULL);

        if (clientFd == -1){
            perror("accept");
            continue;
        }

        while (1){
            uint32_t size;
            char *buffer = get_msg(clientFd, &size);

            if (buffer == NULL)
                break;

            printf("%s\n", buffer);

            int res = analyzeCalls(buffer, clientFd);

            free(buffer);

            if (res == -1)break;
        }

        close(clientFd);
    }

    close(sock);
    return 0;
}

int analyzeCalls(char *req,int fd){
    char listCall[] = "LIST";
    char opendDirCall[] = "GET";
    char addDirCall[] = "PUT";
    char deleteCall[] = "DELETE";
    char newDirCall[] = "MKDIR";
    char exitCall[] = "EXIT";
    char currDir[] = "PWD";


    char *fileName = getArgument(req);
    int bytes=-1;

    if(strcmp(listCall,req) == 0){
        bytes = listAll(fd);
    }else if(strncmp(opendDirCall,req,3) == 0){
        bytes = read_func(fd,fileName);
    }else if(strncmp(addDirCall,req,3) == 0){
        printf("what name of the file you want: ");
        char newFileName[] = "newFile.jpeg" ;
        bytes = get_fileData(fd,newFileName);
    }else if(strncmp(req,deleteCall,6) == 0){
        bytes = delete_func(fileName);
    }else if(strncmp(req,newDirCall,5) == 0){
        bytes = create_dir(fileName);
    }else if(strcmp(req,exitCall) == 0){
        return -1;
    }else if(strcmp(req,currDir) == 0){
        bytes = getWorking_dir(fd); 
    }

    if(bytes == -1) return -1;
    return bytes;
}

// the new functions are being added up here
int create_dir(char *dirName){
    int n = mkdir(dirName,0755);
    return n;
}

int remove_dir(char *dirName){
    int n = rmdir(dirName);
    return n;
}

int change_dir(const char *dirName){
    int n = chdir(dirName);
    return n ;
}
int getWorking_dir(int fd){
    char *dirName = getcwd(NULL, 0);
    if (dirName == NULL)return -1;

    int bytes = send_msg(fd, dirName, strlen(dirName));
    free(dirName);

    return bytes;
}

int delete_func(char *fileName){
    int bytes = remove(fileName);
    return bytes;
}

int listAll(int fd){
    DIR *dir = opendir("."); // opening the dir and using pointer to point at it
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
        if(bytes == -1) return -1;
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