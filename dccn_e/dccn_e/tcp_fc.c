#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 12345
#define BUFFER_SIZE 2048

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char command[BUFFER_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    printf("Connected to FTP Server...\n");

    while (1) {
        printf("Enter command (MKDIR, SAVE, RETR, LIST): ");
        fgets(command, BUFFER_SIZE, stdin);
        command[strlen(command) - 1] = '\0';  // Remove newline character

        send(sock, command, strlen(command), 0);

        if (strncmp(command, "SAVE", 4) == 0) {
            printf("Enter file content to save: ");
            fgets(buffer, BUFFER_SIZE, stdin);
            send(sock, buffer, strlen(buffer), 0);
        } else if (strncmp(command, "RETR", 4) == 0) {
            FILE *file;
            char filename[BUFFER_SIZE];
            printf("Enter file name to retrieve: ");
            fgets(filename, BUFFER_SIZE, stdin);
            filename[strlen(filename) - 1] = '\0';  // Remove newline character
            send(sock, filename, strlen(filename), 0);

            // Receive file content
            file = fopen(filename, "wb");
            if (file == NULL) {
                printf("Failed to open file %s for writing.\n", filename);
                continue;
            }

            int bytes_received;
            while ((bytes_received = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
                if (bytes_received < BUFFER_SIZE) {
                    fwrite(buffer, 1, bytes_received, file);
                    break; // End of file transfer
                }
                fwrite(buffer, 1, bytes_received, file);
            }

            fclose(file);
            printf("File %s retrieved successfully.\n", filename);
        }

        // Receive and print server response
        if (recv(sock, buffer, BUFFER_SIZE, 0) > 0) {
            printf("Server response: %s\n", buffer);
        }
    }

    close(sock);
    return 0;
}