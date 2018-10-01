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

#define GROUP_ID "Y_Project_2_65"

using namespace std;

string fortuneId = "transient bus protocol violation";
map<string, string> usernameMap;
set<int> fdConnected;
map<string, int> usernameFdMap;

/*
    struct KnockModel - incomplete struct to hold which ports a user has knocked on with a timestamp
*/
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

        ports.push_back(newPort);
    }
};

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*
    setHints - addrinfo hints is used in the main function just before initializing the sockets. 
    The function sets the socktype and flags.
*/
void setHints(addrinfo &hints) {
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
}

/*
    populateBindSocket - Creates and binds a socket with a specific port in mind.
*/
int populateBindSocket(const char port[5]) {
    int listener;
    struct addrinfo hints, *addressInfo, *mainSock;  
    int soReuseAddr = 1;

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

    freeaddrinfo(addressInfo); 

    return listener;
}


/*
    processMap - incomplete function which processes the maps made for port knocking
*/
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
/*
    INCOMPLETE
*/
void checkAddressMap(map<string, KnockModel> aMap) {
    if (!aMap.empty()) {
        map<string, KnockModel>::iterator iter = aMap.begin();
        KnockModel model = iter->second;

        for(int i = 0; i < model.ports.size(); i++) {
            cout << model.ports.at(i) << endl;
        }
    } else {
        cout << "the map is empty" << endl;
    }
    
}

/*
    sendMessage - sends the message to approtriate user with specific File Descriptor
*/
void sendMessage(int fd, string message, int cBytes) {
    char * sendCharMsg = new char[message.length()+1];
    strcpy(sendCharMsg, message.c_str());
    if (send(fd, sendCharMsg, strlen(sendCharMsg)+1, 0) == -1) {
        perror("sending error");
    }
}

/*
    isConnectedUser - searches a specific ip-address in a global map container to see if the address
    is already connected.
*/
bool isConnectedUser(string address) {
    if(usernameMap.find(address) == usernameMap.end()) {
        return false;
    } else {
        return true;
    }
}

/*
    changeFortuneId - changes the global fortune variable for the server ID.
*/
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

/*
    getServerId - gets the full server id 
        Format: fortuneId groupId timestamp
*/
string getServerId() {
    string sendId = fortuneId + " " +  GROUP_ID;
    time_t stamp = time(0);
    sendId += " " + string(asctime(localtime(&stamp)));
    return sendId;
}



/*
    checkConnection - the main checker, runs through all sockets to see incoming packages.
    Sends back the appropriate package depending on the incoming message.
*/
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
            perror("ERROR: error in selecting");
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
                        perror("ERROR: did not accept connection");
                    } else {
                        FD_SET(newFd, &mainFd); // add to master set
                        if (newFd > maxFd) {    // keep track of the max
                            maxFd = newFd;
                        }
                        printf("Connection from %s on socket %d\n",
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
                        close(i); // close the connection to user
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

                        // if the package requests connection
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
                            
                        } 
                        // If the package requests leave (exit)
                        else if (msgVector.at(0) == "LEAVE") {
                            map<string, string>::iterator leaveIter = usernameMap.find(address);

                            if( leaveIter != usernameMap.end()) {
                                string username = leaveIter->second;
                                usernameMap.erase(address);
                                usernameFdMap.erase(username);
                                fdConnected.erase(i);
                                
                                close(i); // closes!!!
                                FD_CLR(i, &mainFd); // remove connection from master FD
                            }

                        } 
                        // if the package requests to send a message
                        else if (msgVector.at(0) == "MSG") {
                            map<string, string>::iterator usernameIter = usernameMap.find(address);
                            string messageToSend;
                            if (usernameIter != usernameMap.end()) {
                                messageToSend += usernameIter->second + ": ";
                            }

                            for(int h = 2; h < msgVector.size(); h++) {
                                messageToSend += msgVector.at(h);
                                messageToSend += " ";
                            }

                            // if the package requests to send all users message
                            if (msgVector.at(1) == "ALL") {
                                for(int j = 0; j <= maxFd; j++) {
                                    // send the data
                                    if (FD_ISSET(j, &mainFd)) {
                                        // except to the listener, ourselves and thouse that have not connected
                                        if (j != listener && j != i && fdConnected.find(j) != fdConnected.end()) {
                                            sendMessage(j, messageToSend, clientBytes);
                                        }
                                    }
                                }
                            } 
                            // if the package requests to send only 1 user messgae
                            else {
                                string sendUser = msgVector.at(1);
                                map<string, int>::iterator fdIter = usernameFdMap.find(sendUser);
                                
                                if (fdIter == usernameFdMap.end()) {
                                    sendMessage(i, "There is no username which matches", clientBytes);
                                } else {
                                    sendMessage(fdIter->second, messageToSend, clientBytes);
                                }

                            }
                        } 
                        // If the package requests to view all users on server
                        else if (msgVector.at(0) == "WHO") {
                            
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
                        } 
                        // if the package requests for the uniqe ID
                        else if (messageString == "ID" || msgVector.at(0) == "ID") {
  
                            sendMessage(i, getServerId(), clientBytes);
                        } 
                        // if the package requests to change the unique ID
                        else if (msgVector.at(0) == "CHANGE") {
                            if (msgVector.at(1) == "ID") {
                                cout << "ID has changed" << endl;
                                changeFortuneId();
                                sendMessage(i, getServerId(), clientBytes);
                            } 
                        }
                        // No commands found in package, notify the user
                        else 
                        {
                            string msgSend = "Invalid commands!\nUse: CONNECT, LEAVE, MSG ALL, MSG <username>, WHO, ID, CHANGE ID";
                            sendMessage(i, msgSend, clientBytes);
                        }
                    }
                } 
            } 
        } 
}

/*
    checkKnockPort - this is an incomplete function to check all file descriptors for a specific knocking
    port.
*/
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
            perror("ERROR: error in selecting");
        }

        // look for existing connections
        for(int i = 0; i <= maxFd; i++) {
            /*
                Main Socket check
            */
            // Here we have something coming in
            if (FD_ISSET(i, &readFd)) { 
                if (i == listener) {
                    // handle new connections
                    addrLength = sizeof clientAddr;
                    newFd = accept(listener,
                        (struct sockaddr *)&clientAddr,
                        &addrLength);

                    if (newFd == -1) {
                        perror("ERROR: did not accept connection");
                    } else {
                        FD_SET(newFd, &mainFd); // add to master set
                        if (newFd > maxFd) {    // keep track of the max
                            maxFd = newFd;
                        }

                        string address(inet_ntop(clientAddr.ss_family,
                                get_in_addr((struct sockaddr*)&clientAddr),
                                ipAddress, INET6_ADDRSTRLEN));
                        
                       
                        processMap(refMap, address, port, corrPorts);
                        
                        checkAddressMap(refMap);

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


/*
    The main function
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

    // endless while loop iterates the main checkConnection function
    while(1) {
        checkConnection(masterFd, maxFdMaster, masterListener, addressLength, clientAddress, addressMap, correctPorts);
        //checkKnockPort(knockFd1, maxFdKnock1, listener2, addressLength, clientAddress, addressMap, string(port1), correctPorts);
        //checkKnockPort(knockFd2, maxFdKnock2, listener3, addressLength, clientAddress, addressMap, string(port2),correctPorts);
    }
    return 0;
}