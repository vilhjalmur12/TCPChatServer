# README
# SC-T-409-TSAM
### Project 2 - Chat server
This README is about our simple chat sever(that was not that simple) and with accompanying client.  
The whole system was compiled on Linux Ubuntu.

## Server commands  
ID - Generates unique server ID.  
CONNECT <Name> - To join the chat, you have connect first.  
LEAVE - To leave the server.  
WHO - Lists all the users on the server.  
MSG <Username> - Sends private message to uses you name.  
MSG ALL - Sends message to everybody.  
CHANGE ID - Generates new server id, with timestamp and group's initials(Y_Project_2 65).  

## To build
### Server  
g++ server.cpp -o server  
### Client  
g++ client.cpp -o client
## To Run  
### Server
./server
### Client
./client hostname port


## Known Issues
Port knocking

## Team Members - Y_Project_2 65
Ívar Kristinn Hallsson: ivar17@ru.is  
Vilhjálmur Rúnar vilhjálmsson: vilhjalmur12@ru.is  
