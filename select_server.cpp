#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>

using namespace std;

#define PORT1 "5000"
#define PORT2 "5001"
#define PORT3 "5002"

// TODO: brjóta betur
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void setHints(addrinfo &hints) {
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
}

/*

void bindAll(addrinfo *hints, addrinfo **address, 
                addrinfo &mainSocket, int &listener) {
    if ((getaddrinfo(NULL, PORT1, hints, address)) != 0) {
        cout << "Error: could not read address info: " << hints->ai_addr << endl;
        exit(1);
    }


    for(mainSocket = address; mainSocket != NULL; mainSock = mainSock->ai_next) {
        
        cout << "Doing next Testing " << mainSock->ai_addr << endl;

        listener = socket(mainSock->ai_family, mainSock->ai_socktype, mainSock->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &soReuseAddr, sizeof(int));

        if (bind(listener, mainSock->ai_addr, mainSock->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }
}

*/

int main(int argc, char *argv[])
{
    if(argc < 1) {
        cout << "Use: > ipaddress" << endl;
        exit(1);
    }

    /*
        Initis
    */
   
    // The socket address container
    struct sockaddr_storage clientAddress;
    socklen_t addressLength;
    
    // master and read file descriptor list
    fd_set masterFd, readFd;
    int listener, listener2, listener3, newFd;

    // client buffer
    char clientBuff[256];
    int clientBytes;     

    char ipAddress[INET6_ADDRSTRLEN];

    // Address info structs 
    struct addrinfo hints, *addressInfo, *mainSock;  
    int soReuseAddr = 1;
    
    /* Initis - END */

    /*
        Populate and bind 3 sockets for all ports
    */

    // populate hints bytes
    setHints(hints);

    if ((getaddrinfo(NULL, PORT1, &hints, &addressInfo)) != 0) {
        cout << "Error: could not read address info: " << hints.ai_addr << endl;
        exit(1);
    }

    for(mainSock = addressInfo; mainSock != NULL; mainSock = mainSock->ai_next) {
        
        listener = socket(mainSock->ai_family, mainSock->ai_socktype, mainSock->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &soReuseAddr, sizeof(int));

        if (bind(listener, mainSock->ai_addr, mainSock->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    if (mainSock == NULL) {
        cout << "server failed to bind" << endl;
    }

    freeaddrinfo(addressInfo); // all done with this

    /* Populate and bind - END */

    // listen
    if (listen(listener, 10) == -1) {
        cout << "failed to listen" << endl;
    }

    // add the listener to the master set
    FD_SET(listener, &masterFd);

    // keep track of the biggest file descriptor
    int maxFileDescritpor = listener; // so far, it's this one

    while(1) {
        readFd = masterFd;

        if (select(maxFileDescritpor+1, &readFd, NULL, NULL, NULL) == -1) {
            //cout << "error in selecting" << endl;
        }

        // run through the existing connections looking for data to read
        for(int i = 0; i <= maxFileDescritpor; i++) {
            if (FD_ISSET(i, &readFd)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addressLength = sizeof clientAddress;
                    newFd = accept(listener,
                        (struct sockaddr *)&clientAddress,
                        &addressLength);

                    if (newFd == -1) {
                        //cout << "Error in accepting connection" << endl;
                    } else {
                        FD_SET(newFd, &masterFd); // add to master set
                        if (newFd > maxFileDescritpor) {    // keep track of the max
                            maxFileDescritpor = newFd;
                        }
                        printf("selectserver: new connection from %s on "   // TODO: Skoða betur
                            "socket %d\n",
                            inet_ntop(clientAddress.ss_family,
                                get_in_addr((struct sockaddr*)&clientAddress),
                                ipAddress, INET6_ADDRSTRLEN),
                            newFd);
                    }
                } else {
                    // handle data from a client
                    if ((clientBytes = recv(i, clientBuff, sizeof clientBuff, 0)) <= 0) {
                        // got error or connection closed by client
                        
                        if (clientBytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                            cout << "Connection closed: socket hung up" << endl;
                        } else {
                            
                            cout << "Recived!" << endl;
                            
                        }
                        close(i); // bye!
                        FD_CLR(i, &masterFd); // remove from master set
                    } else {
                    
                        // Hér validatum við message

                        // we got some data from a client
                        for(int j = 0; j <= maxFileDescritpor; j++) {
                            // send to everyone!
                            if (FD_ISSET(j, &masterFd)) {
                                // except the listener and ourselves
                                if (j != listener && j != i) {
                                    if (send(j, clientBuff, clientBytes, 0) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        }
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    }
    return 0;
}