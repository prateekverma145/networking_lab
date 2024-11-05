#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char command[BUFFER_SIZE] = {0};
    socklen_t addr_len = sizeof(serv_addr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("\nSocket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported\n");
        return -1;
    }

    printf("Connected to FTP Server (UDP)...\n");

    while (1) {
        printf("Enter command (MKDIR, SAVE, RETR, LIST): ");
        fgets(command, BUFFER_SIZE, stdin);
        command[strlen(command) - 1] = '\0'; 

        sendto(sockfd, command, strlen(command), 0, (struct sockaddr *)&serv_addr, addr_len);

        if (strncmp(command, "SAVE", 4) == 0) {
            printf("Enter file content to save: ");
            fgets(buffer, BUFFER_SIZE, stdin);
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&serv_addr, addr_len);
        }
        int bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&serv_addr, &addr_len);
        buffer[bytes_received] = '\0';
        printf("Server response: %s\n", buffer);
    }

    close(sockfd);
    return 0;
}