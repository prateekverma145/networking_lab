#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void handle_client(int sockfd, struct sockaddr_in *client_addr, socklen_t addr_len, char *buffer) {
    char client_message[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    
    if (strncmp(buffer, "MKDIR", 5) == 0) {
        char dirname[BUFFER_SIZE];
        sscanf(buffer + 6, "%s", dirname);
        if (mkdir(dirname, 0777) == 0) {
            snprintf(client_message, sizeof(client_message), "Directory %s created.\n", dirname);
        } else {
            snprintf(client_message, sizeof(client_message), "Failed to create directory %s.\n", dirname);
        }
        sendto(sockfd, client_message, strlen(client_message), 0, (struct sockaddr *)client_addr, addr_len);
    } else if (strncmp(buffer, "SAVE", 4) == 0) {
        char filename[BUFFER_SIZE];
        sscanf(buffer + 5, "%s", filename);
        FILE *fp = fopen(filename, "w");
        if (fp) {
            recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)client_addr, &addr_len);  // File content
            fwrite(buffer, sizeof(char), strlen(buffer), fp);
            fclose(fp);
            snprintf(client_message, sizeof(client_message), "File %s saved.\n", filename);
        } else {
            snprintf(client_message, sizeof(client_message), "Failed to save file %s.\n", filename);
        }
        sendto(sockfd, client_message, strlen(client_message), 0, (struct sockaddr *)client_addr, addr_len);
    } else if (strncmp(buffer, "RETR", 4) == 0) {
        char filename[BUFFER_SIZE];
        sscanf(buffer + 5, "%s", filename);
        FILE *fp = fopen(filename, "r");
        if (fp) {
            fread(buffer, sizeof(char), BUFFER_SIZE, fp);
            fclose(fp);
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)client_addr, addr_len);
        } else {
            snprintf(client_message, sizeof(client_message), "Failed to retrieve file %s.\n", filename);
            sendto(sockfd, client_message, strlen(client_message), 0, (struct sockaddr *)client_addr, addr_len);
        }
    } else if (strncmp(buffer, "LIST", 4) == 0) {
        char dirname[BUFFER_SIZE];
        sscanf(buffer + 5, "%s", dirname);
        DIR *d = opendir(dirname);
        struct dirent *dir;
        if (d) {
            strcpy(client_message, "");
            while ((dir = readdir(d)) != NULL) {
                strcat(client_message, dir->d_name);
                strcat(client_message, "\n");
            }
            closedir(d);
            sendto(sockfd, client_message, strlen(client_message), 0, (struct sockaddr *)client_addr, addr_len);
        } else {
            snprintf(client_message, sizeof(client_message), "Failed to list directory %s.\n", dirname);
            sendto(sockfd, client_message, strlen(client_message), 0, (struct sockaddr *)client_addr, addr_len);
        }
    }
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(client_addr);
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("FTP Server (UDP) listening on port %d...\n", PORT);

    while (1) {
        int bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
        buffer[bytes_received] = '\0';
        printf("Command received from client IP: %s, Port: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        handle_client(sockfd, &client_addr, addr_len, buffer);
    }

    close(sockfd);
    return 0;
}