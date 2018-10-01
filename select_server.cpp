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
#include <vector>
#include <ctime>
#include <map>
#include <set>
#include <regex>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <array>

#define GROUP_ID "17"

using namespace std;

string fortuneId = "transient bus protocol violation";
map<string, string> usernameMap;
set<int> fdConnected;
map<string, int> usernameFdMap;

struct KnockModel {
    vector<string> ports;
    time_t timestamp;

    KnockModel(): timestamp(time(0)) { };

    bool portInModel(string newPort) {
        for(int i = 0; i < ports.size(); ++i) {
            if(ports[i] == newPort) {
                return true;
            }
        }
        return false;
    }

    void addPort(string newPort) {
        /*
        if (difftime(time(0), timestamp + (60 * 2)) > 0) {
            return 0;
        }
        if (portInModel(newPort)) {
            return 1;
        }
        */ 

        ports.push_back(newPort);
    }
};

// TODO: skoða betur
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

int populateBindSocket(const char port[5]) {
    int listener;
    struct addrinfo hints, *addressInfo, *mainSock;  
    int soReuseAddr = 1;

    /*
        Populate and bind 3 sockets for all ports
    */

    // populate hints bytes
    setHints(hints);

    if ((getaddrinfo(NULL, port, &hints, &addressInfo)) != 0) {
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

    return listener;
}

bool processMap(map<string, KnockModel> &addressMap, string ipAddress, string port, vector<string> corrPorts) {
    struct KnockModel tmpKnock;
    tmpKnock.addPort(port);
   
    if(addressMap.insert(pair <string, KnockModel> (ipAddress, tmpKnock)).second != false) {
        // there is no ip address in there
        
        return false;
    } else {
        map<string, KnockModel>::iterator it = addressMap.find(ipAddress);
        KnockModel &model = it->second;
        
        if(model.ports.size() < 3) {
            it->second.addPort(port);
            return false;
        }

        for(int i = 0; i < model.ports.size(); i++) {
            if (model.ports.at(i).compare(corrPorts.at(i)) != 0) {
                addressMap.erase(it->first);
                return false;
            }
        }
        return true;
    }
}

void checkAddressMap(map<string, KnockModel> aMap) {
    if (!aMap.empty()) {
        map<string, KnockModel>::iterator iter = aMap.begin();
        KnockModel model = iter->second;
        
        // TESTY
        cout << "Address check: "<< string(iter->first) << endl;

        for(int i = 0; i < model.ports.size(); i++) {
            cout << model.ports.at(i) << endl;
        }
    } else {
        cout << "the map is empty" << endl;
    }
    
}

void sendMessage(int fd, string message, int cBytes) {
    char * sendCharMsg = new char[message.length()+1];
    strcpy(sendCharMsg, message.c_str());
    if (send(fd, sendCharMsg, strlen(sendCharMsg)+1, 0) == -1) {
        perror("sending error");
    }
}

bool isConnectedUser(string address) {
    if(usernameMap.find(address) == usernameMap.end()) {
        return false;
    } else {
        return true;
    }
}

void changeFortuneId() {
    string command("fortune -s");
    array<char, 128> buffer;


    FILE* pipe = popen(command.c_str(), "r");
    if(pipe) {
        fortuneId = "";
        while (fgets(buffer.data(), 128, pipe) != NULL) {
            std::cout << "Reading..." << std::endl;
            fortuneId += buffer.data();
        }
    }
}

string getServerId() {
    string sendId = fortuneId + " " +  GROUP_ID;
    time_t stamp = time(0);
    sendId += " " + string(asctime(localtime(&stamp)));
    return sendId;
}

void checkConnection(fd_set &mainFd, int &maxFd, int listener, 
                        socklen_t &addrLength, sockaddr_storage &clientAddr, map<string, 
                        KnockModel> &refMap, vector<string> corrPorts) {
    fd_set readFd = mainFd;
    int newFd;
    char ipAddress[INET6_ADDRSTRLEN];
    // client buffer
    char clientBuff[256];
    int clientBytes;

        if (select(maxFd+1, &readFd, NULL, NULL, NULL) == -1) {
            //cout << "error in selecting" << endl;
        }

        // run through the existing connections looking for data to read
        for(int i = 0; i <= maxFd; i++) {
            /*
                Main Socket check
            */
            if (FD_ISSET(i, &readFd)) {
                
                            
                if (i == listener) {
                    // handle new connections
                    addrLength = sizeof clientAddr;
                    newFd = accept(listener,
                        (struct sockaddr *)&clientAddr,
                        &addrLength);

                    if (newFd == -1) {
                        //cout << "Error in accepting connection" << endl;
                    } else {
                        FD_SET(newFd, &mainFd); // add to master set
                        if (newFd > maxFd) {    // keep track of the max
                            maxFd = newFd;
                        }
                        printf("selectserver: new connection from %s on "   // TODO: Skoða betur
                            "socket %d\n",
                            inet_ntop(clientAddr.ss_family,
                                get_in_addr((struct sockaddr*)&clientAddr),
                                ipAddress, INET6_ADDRSTRLEN),
                            newFd);
                    
                        string address(inet_ntop(clientAddr.ss_family,
                                get_in_addr((struct sockaddr*)&clientAddr),
                                ipAddress, INET6_ADDRSTRLEN));

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
                        FD_CLR(i, &mainFd); // remove from master set
                    } else {
                        string address(inet_ntop(clientAddr.ss_family,
                                get_in_addr((struct sockaddr*)&clientAddr),
                                ipAddress, INET6_ADDRSTRLEN));


                        

                        string messageString = string(clientBuff);

                        vector<string> msgVector;
                        istringstream iss(messageString);
                        string msgSend;
                        for(string messageString; iss >> messageString;) {
                            msgVector.push_back(messageString);
                        }

                        if (msgVector.at(0) == "CONNECT") {
                            string username = msgVector.at(1);

                            if(usernameMap.find(address) != usernameMap.end()) {
                                map<string, string>::iterator uMapIter = usernameMap.find(address);
                                username = uMapIter->second;
                                msgSend = "You are already connected with the username " + username;
                                sendMessage(i, msgSend, clientBytes);
                            } 
                            else if (usernameFdMap.find(username) != usernameFdMap.end()) {
                                string msg = "Sorry the username is taken";
                                sendMessage(i, msg, clientBytes);
                                close(i);
                                FD_CLR(i, &mainFd); // remove from master set
                            } 
                            else {
                                usernameMap.insert(pair<string, string> (address, username));
                                fdConnected.insert(i);
                                usernameFdMap.insert(pair<string, int> (username, i));

                                cout << "User: " << username << " CONNECTED" << endl;
                            }
                            
                        } else if (msgVector.at(0) == "LEAVE") {
                            map<string, string>::iterator leaveIter = usernameMap.find(address);

                            if( leaveIter != usernameMap.end()) {
                                string username = leaveIter->second;
                                usernameMap.erase(address);
                                usernameFdMap.erase(username);
                                fdConnected.erase(i);
                                
                                close(i); // closes!!!
                                FD_CLR(i, &mainFd); // remove connection from master FD
                            }

                        } else if (msgVector.at(0) == "MSG") {
                            map<string, string>::iterator usernameIter = usernameMap.find(address);
                            string messageToSend;
                            if (usernameIter != usernameMap.end()) {
                                messageToSend += usernameIter->second + ": ";
                            }

                            for(int h = 2; h < msgVector.size(); h++) {
                                messageToSend += msgVector.at(h);
                                messageToSend += " ";
                            }

                            if (msgVector.at(1) == "ALL") {
                                // we got some data from a client
                                for(int j = 0; j <= maxFd; j++) {
                                    // send to everyone!
                                    if (FD_ISSET(j, &mainFd)) {
                                        // except the listener, ourselves and thouse that have not connected
                                        if (j != listener && j != i && fdConnected.find(j) != fdConnected.end()) {
                                            sendMessage(j, messageToSend, clientBytes);
                                        }
                                    }
                                }
                            } else {
                                string sendUser = msgVector.at(1);
                                map<string, int>::iterator fdIter = usernameFdMap.find(sendUser);
                                
                                if (fdIter == usernameFdMap.end()) {
                                    sendMessage(i, "There is no username which matches", clientBytes);
                                } else {
                                    sendMessage(fdIter->second, messageToSend, clientBytes);
                                }

                            }
                        } else if (msgVector.at(0) == "USER") {
                            
                            if(usernameMap.empty()) {
                                sendMessage(i, "No users", clientBytes);
                            } else {
                                string sendValue = "USERS: ";                            
                               map<string, string>::iterator userIter = usernameMap.begin();

                               for(userIter; userIter != usernameMap.end(); userIter++) {
                
                                   sendValue += userIter->second + " ";
                               } 

                               sendMessage(i, sendValue, clientBytes);
                            }
                        } else if (messageString == "ID" || msgVector.at(0) == "ID") {
  
                            sendMessage(i, getServerId(), clientBytes);
                        } else if (msgVector.at(0) == "CHANGE") {
                            if (msgVector.at(1) == "ID") {
                                cout << "ID has changed" << endl;
                                changeFortuneId();
                                sendMessage(i, getServerId(), clientBytes);
                            }
                            
                        }

                        

                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
}


void checkKnockPort(fd_set &mainFd, int &maxFd, int listener, 
                        socklen_t &addrLength, sockaddr_storage &clientAddr, map<string, 
                        KnockModel> &refMap, string port,  vector<string> corrPorts) {
    fd_set readFd = mainFd;
    int newFd;
    char ipAddress[INET6_ADDRSTRLEN];
    // client buffer
    char clientBuff[512];
    int clientBytes; 


        if (select(maxFd+1, &readFd, NULL, NULL, NULL) == -1) {
            //cout << "error in selecting" << endl;
        }

        // run through the existing connections looking for data to read
        for(int i = 0; i <= maxFd; i++) {
            /*
                Main Socket check
            */
            if (FD_ISSET(i, &readFd)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrLength = sizeof clientAddr;
                    newFd = accept(listener,
                        (struct sockaddr *)&clientAddr,
                        &addrLength);

                    if (newFd == -1) {
                        //cout << "Error in accepting connection" << endl;
                    } else {
                        FD_SET(newFd, &mainFd); // add to master set
                        if (newFd > maxFd) {    // keep track of the max
                            maxFd = newFd;
                        }

                        string address(inet_ntop(clientAddr.ss_family,
                                get_in_addr((struct sockaddr*)&clientAddr),
                                ipAddress, INET6_ADDRSTRLEN));
                            
                        /*
                        cout << "the fucking address: " << address << endl;
                        if (strcmp(address, "127.0.0.1") == 0) {
                            cout << "its him!" << endl;
                        }

                        */
                       
                        processMap(refMap, address, port, corrPorts);
                        
                        checkAddressMap(refMap);
                        
                        /*
                        map<const char *, KnockModel>::iterator iter = refMap.find(ipAddress);

                        if (iter != refMap.end()) {
                            cout << iter->second.timestamp << endl;
                        }
                         */

                        cout << "Your request has been handled" << endl;
                        printf("knockserver: new connection from %s on "   // TODO: Skoða betur
                            "socket %d\n", address, newFd);
                    
                    }
                }
                close(i); // bye!
                FD_CLR(i, &mainFd); // remove from master set
            }
            
        }
}



int main(int argc, char *argv[])
{
    if(argc < 1) {
        cout << "Use: > ipaddress" << endl;
        exit(1);
    }

    /*
        Initis
    */
    map<string, KnockModel> addressMap;

    const char mainPort[5] = "5001";
    const char port1[5] = "5002";
    const char port2[5] = "5003";

    vector<string> correctPorts;
    correctPorts.push_back(string(port1));
    correctPorts.push_back(string(port2));
    correctPorts.push_back(string(mainPort));

    // The socket address container
    struct sockaddr_storage clientAddress;
    socklen_t addressLength;
    
    // master and read file descriptor list
    fd_set masterFd, readFd, knockFd1, knockFd2;
    int newFd;

    // client buffer
    char clientBuff[256];
    int clientBytes;     

    char ipAddress[INET6_ADDRSTRLEN];

    int masterListener = populateBindSocket(mainPort);
    int listener2 = populateBindSocket(port1);
    int listener3 = populateBindSocket(port2);

    // listen
    if (listen(masterListener, 10) == -1) {
        cout << "failed to listen on main" << endl;
    }

    if (listen(listener2, 10) == -1) {
        cout << "failed to listen on 2" << endl;
    }

    if (listen(listener3, 10) == -1) {
        cout << "failed to listen on 3" << endl;
    }

    // add the listener to the master set
    FD_SET(masterListener, &masterFd);
    FD_SET(listener2, &knockFd1);
    FD_SET(listener3, &knockFd2);

    // keep track of the biggest file descriptor
    int maxFdMaster = masterListener; // so far, it's this one
    int maxFdKnock1 = listener2;
    int maxFdKnock2 = listener3;


    while(1) {
        checkConnection(masterFd, maxFdMaster, masterListener, addressLength, clientAddress, addressMap, correctPorts);
        checkKnockPort(knockFd1, maxFdKnock1, listener2, addressLength, clientAddress, addressMap, string(port1), correctPorts);
        checkKnockPort(knockFd2, maxFdKnock2, listener3, addressLength, clientAddress, addressMap, string(port2),correctPorts);
    }
    return 0;
}