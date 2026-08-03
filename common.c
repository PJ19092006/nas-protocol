#include "common.h"


void errNClose(const char *msg,int fd){
    close(fd);
    perror(msg);
    exit(EXIT_FAILURE);
}

int createSocket(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == -1) {
        perror("socket");
        return -1;
    }
    return sock;  
}

uint32_t getHeader(int client_fd){
    uint32_t netLength ;
    int res = recvHelper(client_fd,&netLength,sizeof(netLength));
    if (res == -1) return UINT32_MAX;
    uint32_t length = ntohl(netLength);

    return length;
}

// sending helper methods
int send_msg (int fd , const void *buffer, uint32_t length){
    uint32_t lengthMsg = htonl(length);

    // sending just the length (headder)
    int lengthBytes = sendRecursively(fd,&lengthMsg,sizeof(lengthMsg));
    if (lengthBytes == -1) return -1;

    // sending the msg (body)
    int msgBytes = sendRecursively(fd, buffer, length);
    if (msgBytes == -1) return -1;

    return lengthBytes + msgBytes;
}

int sendRecursively(int sock, const void *buffer, uint32_t length){
    const char *ptr = buffer;
    size_t remaining = length;

    while(remaining > 0){
        size_t sentBytes = write(sock, ptr,remaining);
        if (sentBytes <= 0) return -1;
        remaining -= sentBytes;
        ptr += sentBytes;
    }

    return length;
}

// all good above this

// get string methods
char *get_msg(int fd){
    // extracting headder (length)
    int length = getHeader(fd);

    // alocating memory
    char *buffer;
    buffer = (char *) malloc((length) + 1);
    if(buffer == NULL) return NULL;

    // recive the msg
    int bytes = recvHelper(fd,buffer,(length));
    if(bytes == -1){
        free(buffer);
        return NULL;
    }

    buffer[bytes] = '\0';

    return buffer;
}

int getFiles(int fd){
    uint32_t totalFiles = getHeader(fd);

    while(totalFiles>0){
        char *store = get_msg(fd);
        if(store == NULL) return -1;

        printf("%s\n",store);
        totalFiles--;
        free(store);
    }

    return 0;
}

int recvHelper(int fd, void *buffer, size_t length){
    char *ptr = buffer;
    ssize_t remaining = length;
    int bytes = 0;

    while(remaining > 0){
        ssize_t recivedBytes = read(fd, ptr,remaining);
        if (recivedBytes <= 0) return -1;
        bytes+= recivedBytes;
        remaining -= recivedBytes;
        ptr += recivedBytes;
    }
    return bytes;
}


// working on this function
int send_file(int fd, char *fileName){
    Response res;

    uint64_t size = htonl(get_size(fileName));
    if(size == -1) return -1;


    int totalBytesRead = 0;
	int fileFd = open(fileName, O_RDONLY);
    
    if(size == -1 || fileFd == -1){
        res.status = STATUS_ERROR;
        sendRecursively(fd, &res, sizeof(res));
        return -1;
    }

    res.status = STATUS_OK;
    sendRecursively(fd, &res, sizeof(res));
    // sending size
    sendRecursively(fd,&size,sizeof(size));

    char ch[4096];
    int bytesRead;

    while((bytesRead = read(fileFd,ch,sizeof(ch))) > 0){
        totalBytesRead += bytesRead;
        //  here are the new changes 
        int bytes = sendRecursively(fd,ch,bytesRead);
    }

    close(fileFd);
    return totalBytesRead;
}

int get_fileData(int sock, char fileName[]){
    uint64_t length;
    recvHelper(sock, &length, sizeof(length));
    
    uint64_t remaining = length;
    int fileFd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC,0644);
    if (fileFd == -1) return -1;

    
    while(remaining > 0){
        char ch[4096];
        size_t chunk = remaining > sizeof(ch)? sizeof(ch): remaining;
        int bytes = recvHelper(sock,ch,chunk);
        if(bytes == -1) return -1;
        int n = write(fileFd,ch,bytes);
        remaining -= bytes;
    }
    

    close(fileFd);
    return length;
}

size_t get_size(char *fileName){
    struct stat info; // Stat structure using it as info
    size_t size = -1;

	int n = stat(fileName,&info);
	if(n == 0){
        size = info.st_size;
	}

    return size;
}

char *getArgument(char *req){
    char *space = strchr(req, ' ');

    if (space == NULL)return NULL;

    return space + 1;
}