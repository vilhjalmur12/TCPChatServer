#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <iostream>
using namespace std;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

//receive message
void *recvmessage(void *sock)
{
    int sockfd = *((int *)sock);
    char buf[256];
    int i;
    while ((i = recv(sockfd, buf, 256, 0)) > 0)
    {
        buf[i] = '\0';
        cout << string(buf);
        memset(buf, '\0', sizeof(buf));
    }
}

int main(int argc, char *argv[])
{
    //variables
    int sock_fd, portno;
    pthread_t recthread;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];

    //verify args
    if (argc < 3)
    {
        fprintf(stderr, "Proper usage is %s hostname port\n", argv[0]);
        exit(0);
    }

    //init connection
    portno = atoi(argv[2]);
    sock_fd = socket(AF_INET, SOCK_STREAM, 0); //establish socket
    if (sock_fd < 0) {
        error("socket");
    }
    
    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port = htons(portno);
    int n = connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)); // connect to the server
    if(n < 0) {
        error("ERROR connecting");
    }

    //recthread created
    pthread_create(&recthread, NULL, recvmessage, &sock_fd);

    // Write to socket
    while (1)
    {
        bzero(buffer, 256);
        fgets(buffer, 255, stdin);
        int n = write(sock_fd, buffer, strlen(buffer)); //send msg
        if (n < 0) {
            error("ERROR writing to socket");
        }
    }
    //recthread joined
    if(pthread_join(recthread, NULL)) {
        error("Error joining thread\n");
        return 2;
    }
    // close the connection
    close(sock_fd);
    return 0;
}