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

//  sending helper methods
int send_msg (int fd , const void *buffer, uint32_t length){
    uint32_t lenghtMsg = htonl(length);

    // sending just the length (headder)
    int lengthBytes = sendRecursively(fd,&lenghtMsg,sizeof(lenghtMsg));
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
char *get_msg(int fd, uint32_t *length){
    // extracting headder (length)
    *length  = getHeader(fd);
    // if(*length > 1000) return NULL; // overlimit memory call

    // alocating memory
    char *buffer;
    buffer = (char *) malloc((*length) + 1);
    if(buffer == NULL) return NULL;

    // recive the msg
    int bytes = recvHelper(fd,buffer,(*length));
    if(bytes == -1){
        free(buffer);
        return NULL;
    }

    buffer[bytes] = '\0';

    return buffer;
}

int getFiles(int fd){
    uint32_t totalFiles = getHeader(fd);
    uint32_t size;

    while(totalFiles>0){
        char *store = get_msg(fd,&size);
        if(store == NULL) return -1;

        printf("%s\n",store);
        totalFiles--;
        free(store);
    }
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