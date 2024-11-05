#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <unistd.h>

#define MAX 1024
#define PORT 16001
#define SA struct sockaddr 

void send_list_of_numbers(int sockfd) {
    char buff[MAX];
    printf("\033[0;33m");
    printf("Enter numbers separated by spaces: ");
    fgets(buff, sizeof(buff), stdin);
    write(sockfd, buff, strlen(buff));
    
    bzero(buff, sizeof(buff));
    read(sockfd, buff, sizeof(buff));
    printf("From Server: %s", buff);
}

void send_list_of_strings(int sockfd, char request_type, const char* prompt) {
    char buff[MAX];
    printf("%s", prompt);
    fgets(buff + 2, sizeof(buff) - 2, stdin);
    buff[0] = request_type;
    buff[1] = ' ';
    write(sockfd, buff, strlen(buff));
    
    bzero(buff, sizeof(buff));
    read(sockfd, buff, sizeof(buff));
    printf("From Server: %s", buff);
}

void func(int sockfd) {
    while (1) {
        printf("\033[0;32m");
        printf("\n1. Send list of numbers\n");
        printf("2. Send list of strings (palindrome check)\n");
        printf("3. Send list of strings (longest string)\n");
        printf("4. Exit\n");
        printf("Enter choice: ");
        
        int choice;
        scanf("%d", &choice);
        getchar(); // Consume newline
        
        switch(choice) {
            case 1:
                send_list_of_numbers(sockfd);
                break;
            case 2:
                send_list_of_strings(sockfd, 'P', "Enter strings separated by spaces (palindrome check): ");
                break;
            case 3:
                send_list_of_strings(sockfd, 'L', "Enter strings separated by spaces (longest string): ");
                break;
            case 4:
                write(sockfd, "exit", 4);
                printf("Client Exit...\n");
                return;
            default:
                printf("Invalid choice. Try again.\n");
        }
    }
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    printf("connected to the server..\n");

    func(sockfd);

    close(sockfd);
    return 0;
}