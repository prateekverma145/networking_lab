#include <stdio.h>
#include <unistd.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#define MAX 80 
#define PORT 16001 
#define SA struct sockaddr 

void HandleRequest(int sockfd) 
{ 
	char buff[MAX]; 
	for (;;) 
	{ 
		bzero(buff, MAX); 
		read(sockfd, buff, sizeof(buff)); 
		if (strncmp("exit", buff, 4) == 0) { 
			printf("client is Exiting...\n");
			bzero(buff, MAX);
		        close(sockfd);	
			break;
		}
	        else
		{

			int inp=atoi(buff);
			bzero(buff, MAX); 
		        printf("\nclient input :%d",inp);	
			if (inp%2==0)
			{
				strcpy(buff,"EVEN");	
				write(sockfd, buff, sizeof(buff)); 
			}
			else
			{
				strcpy(buff,"ODD");	
				write(sockfd, buff, sizeof(buff)); 
			}
		}

	} 
} 

// Driver function 
int main() 
{ 
	int sockfd, connfd, len; 
	struct sockaddr_in servaddr, cli; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("socket creation failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(PORT); 

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
		printf("socket bind failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully binded..\n"); 

	// Now server is ready to listen and verification 
	if ((listen(sockfd, 5)) != 0) { 
		printf("Listen failed...\n"); 
		exit(0); 
	} 
	else
		printf("Server listening..\n"); 
	len = sizeof(cli); 

	// Accept the data packet from client and verification 
	for (;;)
	{	
		connfd = accept(sockfd, (SA*)&cli, &len); 
		printf("Connection accepted from %s:%d\n", inet_ntoa(cli.sin_addr), ntohs(cli.sin_port));
		if (connfd < 0) { 
			printf("server acccept failed...\n"); 
			exit(0); 
		} 
		else
		{	if (fork()==0)
			{
				close(sockfd);
				printf("server acccept the client...\n"); 
				HandleRequest(connfd); 
			}
		}
	}

	// After chatting close the socket 
	close(sockfd); 
} 

