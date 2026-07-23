#include "common.h"

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

    // all good above this

    // listning for dir (response)
    uint32_t size;
    char *file = get_msg(sock,&size);
	int fileFd = open("newImg.jpeg", O_WRONLY | O_CREAT | O_TRUNC,0644);
    if (file == NULL) errNClose("read",sock);

    int n = write(fileFd,file,size);

    close(fileFd);
    free(file);
    close(sock);
    return 0;
}


int connectServer(int sock){
    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(5000);
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); // addr of client

    int connectRes = connect(sock,(struct sockaddr *)&server,sizeof(server));

    return connectRes;
}