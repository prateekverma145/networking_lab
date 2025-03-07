#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <unistd.h>
#include <arpa/inet.h>
#define MAX 80 
#define PORT 12345
#define SA struct sockaddr 
void func(int sockfd) 
{ 
    char buff[MAX]; 
    int n; 

    for (;;) {

        printf("\nEnter the input"); 
        bzero(buff, sizeof(buff)); 
        n = 0; 
        while ((buff[n++] = getchar()) != '\n') 
            ; 

        if ((strncmp(buff, "exit", 4)) == 0) { 
            printf("Client Exit...\n"); 
                write(sockfd, buff, sizeof(buff)); 
            break; 
        } 
        write(sockfd, buff, sizeof(buff)); 
        bzero(buff, sizeof(buff)); 
        read(sockfd, buff, sizeof(buff)); 
        printf("From Server : %s", buff); 
    } 
} 

int main() 
{ 
    int sockfd, connfd; 
    struct sockaddr_in servaddr, cli; 

    // socket create and varification 
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
    servaddr.sin_addr.s_addr = inet_addr("172.16.25.64"); 
    servaddr.sin_port = htons(PORT); 

    // connect the client socket to server socket 
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
        printf("connection with the server failed...\n"); 
        exit(0); 
    } 
    else
        printf("connected to the server..\n"); 

    // function for chat 
    func(sockfd); 

    // close the socket 
    close(sockfd); 
} 