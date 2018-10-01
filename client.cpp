#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
using namespace std;
#define MAXCHARS 1024
typedef struct sockaddr_in SA_in;
typedef struct sockaddr SA;
int port;

/*
Sends a message func
*/
void sendmessage(int sock_fd) {
	char msg_buf[MAXCHARS];

	fgets(msg_buf, MAXCHARS, stdin);
	char full_msg[MAXCHARS] = {'0'};
	snprintf(full_msg, sizeof(msg_buf),"%s", msg_buf);
	write(sock_fd, full_msg, strlen(full_msg));
} 

/*
Receive a message func
*/
void receivesmessage(int sock_fd) {
	char msg_buf[MAXCHARS];
	int n = read(sock_fd, msg_buf, MAXCHARS);
	msg_buf[n - 1] = '\0';
	printf("%s\n", msg_buf);
}


/*
Sends message and receives message from others
*/
void client(fd_set* cli_fds, fd_set* main_fd, int* max_fd, int* sock_fd) {
	*max_fd = *sock_fd;

	int i;
	for (;;)
	{
		*cli_fds = *main_fd;
		select(*max_fd+1, cli_fds, NULL, NULL, NULL);

		for (i = 0; i < *max_fd + 1; ++i)
			if (FD_ISSET(i, cli_fds))
				if (i == 0) {
					sendmessage(*sock_fd);
				}	
				else {
					receivesmessage(*sock_fd);
				}		
	}
} 

/*
TCP socket created and init connection with the server
*/
void starConn(char* ip_addr, fd_set* main_fd, fd_set* cli_fds, int* sock_fd, SA_in* serv_addr)
{
	*sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(serv_addr, sizeof(SA_in));
	serv_addr->sin_family = AF_INET;
	serv_addr->sin_port = htons(port);
	inet_pton(AF_INET, ip_addr, &(serv_addr->sin_addr));
	connect(*sock_fd, (SA*)serv_addr, sizeof(SA));
	printf("Please enter CONNECT <Username>.\n\n");	
	printf("Now you have connected to %s.\n", ip_addr);

	FD_ZERO(main_fd);
    FD_SET(0, main_fd);
	FD_ZERO(cli_fds);
	FD_SET(*sock_fd, main_fd);	
}

/*
 	Main function
 */
int main(int argc, char** argv)
{
	//variables
	int sock_fd, max_fd;
	fd_set main_fd, cli_fds;
	SA_in serv_addr;

	// args
	if (argc != 3)
	{
		printf("Proper usage: client <IP address> <portnumber>\n");
		exit(1);
	}

	port = atoi(argv[2]);
	
	starConn(argv[1], &main_fd, &cli_fds, &sock_fd, &serv_addr);
	client(&cli_fds, &main_fd, &max_fd, &sock_fd);

	// Close the connection
	close(sock_fd);
	return 0;
}