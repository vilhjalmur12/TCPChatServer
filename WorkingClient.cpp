#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
using namespace std;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

string userIdPrompt() {
    string id;
    cout << "CONNECT <USERNAME>" << endl;
    cin >> id;
    string ret = "CONNECT <" + id + ">";

    return ret;
}

int main(int argc, char *argv[])
{
    int sockfd1, sockfd2, sockfd3, portno1, portno2, portno3, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    //test
    pthread_t receiveThread;

    char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(EXIT_FAILURE);
    }

    portno1 = atoi(argv[2]);
   // portno2 = atoi(argv[3]);
   // portno3 = atoi(argv[4]);

    sockfd1 = socket(AF_INET, SOCK_STREAM, 0);
   // sockfd2 = socket(AF_INET, SOCK_STREAM, 0);
   // sockfd3 = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd1 < 0) {
        error("ERROR opening socket");
    }

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(EXIT_FAILURE);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;

    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
   

    cout << " NAME TEST" << endl;
    string userID;
    //userID = userIdPrompt();

    //cout << userID << endl;
    


    //Try to connect to the server/portknock
    serv_addr.sin_port = htons(portno1);
    if (connect(sockfd1,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
    }

  /*  serv_addr.sin_port = htons(portno2);
    if (connect(sockfd2,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
    }

     serv_addr.sin_port = htons(portno3);
    if (connect(sockfd3,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
    }
*/
    cout << "You are connected to the server" << endl;
    
    
//Chat dosent work like it is supposed to work
    while(1) {
        //cout << "> ";
        bzero(buffer,256);
        fgets(buffer,255,stdin);
        n = write(sockfd1,buffer,strlen(buffer));
        if (n < 0) {
            error("ERROR writing to socket");
        }
           

        bzero(buffer,256);
        n = read(sockfd1,buffer,256);
        if (n < 0) {
            error("ERROR reading from socket");
        }

        printf("%s\n",buffer);
    }    
    close(sockfd1);
    return 0;
}