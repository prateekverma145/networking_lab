#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX 1024
#define PORT 12345
#define SA struct sockaddr

void func(int sockfd)
{
    char buff[MAX];
    int n;

    for (;;)
    {
        printf("\n1. Send list of numbers to get count of even and odd numbers.");
        printf("\n2. Send list of strings to get count of palindromes.");
        printf("\n3. Send list of strings to get the longest string.");
        printf("\n4. Exit.");
        printf("\nEnter your choice: ");
        int choice;
        scanf("%d", &choice);
        getchar(); // consume the newline character

        switch (choice)
        {
        case 1:
            printf("Enter list of numbers separated by space: ");
            bzero(buff, sizeof(buff));
            strcpy(buff, "NUMBERS ");
            fgets(buff + 8, sizeof(buff) - 8, stdin);
            buff[strcspn(buff, "\n")] = 0; // remove newline character
            write(sockfd, buff, sizeof(buff));
            break;

        case 2:
            printf("Enter list of strings separated by space: ");
            bzero(buff, sizeof(buff));
            strcpy(buff, "STRINGS_PALINDROME ");
            fgets(buff + 19, sizeof(buff) - 19, stdin);
            buff[strcspn(buff, "\n")] = 0; // remove newline character
            write(sockfd, buff, sizeof(buff));
            break;

        case 3:
            printf("Enter list of strings separated by space: ");
            bzero(buff, sizeof(buff));
            strcpy(buff, "STRINGS_LONGEST ");
            fgets(buff + 16, sizeof(buff) - 16, stdin);
            buff[strcspn(buff, "\n")] = 0; // remove newline character
            write(sockfd, buff, sizeof(buff));
            break;

        case 4:
            strcpy(buff, "exit");
            write(sockfd, buff, sizeof(buff));
            printf("Client Exit...\n");
            return;

        default:
            printf("Invalid choice. Try again.\n");
            continue;
        }

        bzero(buff, sizeof(buff));
        read(sockfd, buff, sizeof(buff));
        printf("From Server: %s\n", buff);
    }
}

int main()
{
    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("Socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("172.16.25.64"); // Replace with server IP
    servaddr.sin_port = htons(PORT);

    if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) != 0)
    {
        printf("Connection with the server failed...\n");
        exit(0);
    }
    else
        printf("Connected to the server..\n");

    func(sockfd);

    close(sockfd);
}
