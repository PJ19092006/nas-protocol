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

int send_file(int fd, char *fileName){
    Response res;

    off_t fileSize = get_size(fileName);
    if(fileSize == (off_t)-1) {
        res.status = STATUS_ERROR;
        sendRecursively(fd, &res, sizeof(res));
        return -1;
    }

    int fileFd = open(fileName, O_RDONLY);
    if(fileFd == -1){
        res.status = STATUS_ERROR;
        sendRecursively(fd, &res, sizeof(res));
        return -1;
    }

    res.status = STATUS_OK;
    if (sendRecursively(fd, &res, sizeof(res)) == -1) {
        close(fileFd);
        return -1;
    }

    // Send 64-bit file size in big-endian network byte order
    uint64_t networkSize = htobe64((uint64_t)fileSize);
    if (sendRecursively(fd, &networkSize, sizeof(networkSize)) == -1) {
        close(fileFd);
        return -1;
    }

    char ch[4096];
    off_t totalBytesRead = 0;
    ssize_t bytesRead;

    while((bytesRead = read(fileFd, ch, sizeof(ch))) > 0){
        if (sendRecursively(fd, ch, bytesRead) == -1) {
            close(fileFd);
            return -1;
        }
        totalBytesRead += bytesRead;
    }

    close(fileFd);
    return (int)totalBytesRead;
}

int get_fileData(int sock, char fileName[]){
    uint64_t networkLength;
    int bytes = recvHelper(sock, &networkLength, sizeof(networkLength));
    
    Response res;
    int fileFd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC,0644);
    if (fileFd == -1 || bytes == -1) {
        res.status = STATUS_ERROR;
        sendRecursively(sock, &res, sizeof(res));
        return -1;
    }

    uint64_t length = be64toh(networkLength);
    uint64_t remaining = length;

    while(remaining > 0){
        char ch[4096];
        size_t chunk = remaining > sizeof(ch)? sizeof(ch): remaining;
        int bytes = recvHelper(sock,ch,chunk);
        if(bytes == -1) return -1;
        ssize_t written = write(fileFd, ch, bytes);
        if(written != bytes){
            close(fileFd);
            return -1;
        }
        remaining -= written;
    }

    res.status = STATUS_OK;
    sendRecursively(sock, &res, sizeof(res));
    close(fileFd);
    return length;
}

off_t get_size(char *fileName){
    struct stat info;
    if (stat(fileName, &info) != 0) {
        return -1;
    }
    return info.st_size;
}

char *getArgument(char *req){
    char *space = strchr(req, ' ');

    if (space == NULL)return NULL;

    return space + 1;
}