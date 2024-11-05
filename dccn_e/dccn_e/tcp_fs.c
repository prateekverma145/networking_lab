#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>

#define PORT 12345
#define BUFFER_SIZE 2048
#define MAX_FILE_SIZE 1024  // 1KB limit

// Function to check if a file already exists
int fileExists(const char *filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

void *handle_client(void *arg) {
    int new_socket = *((int *)arg);
    free(arg);
    char buffer[BUFFER_SIZE];
    char client_message[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = recv(new_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Command received: %s\n", buffer);

        if (strncmp(buffer, "MKDIR", 5) == 0) {
            char dirname[BUFFER_SIZE];
            sscanf(buffer + 6, "%s", dirname);
            if (mkdir(dirname, 0777) == 0) {
                snprintf(client_message, sizeof(client_message), "Directory %s created.\n", dirname);
            } else {
                snprintf(client_message, sizeof(client_message), "Failed to create directory %s.\n", dirname);
            }
            send(new_socket, client_message, strlen(client_message), 0);
        } else if (strncmp(buffer, "SAVE", 4) == 0) {
            char filename[BUFFER_SIZE];
            sscanf(buffer + 5, "%s", filename);
            FILE *fp = fopen(filename, "w");
            if (fp) {
                recv(new_socket, buffer, BUFFER_SIZE, 0);  
                fwrite(buffer, sizeof(char), strlen(buffer), fp);
                fclose(fp);
                snprintf(client_message, sizeof(client_message), "File %s saved.\n", filename);
            } else {
                snprintf(client_message, sizeof(client_message), "Failed to save file %s.\n", filename);
            }
            send(new_socket, client_message, strlen(client_message), 0);
        } else if (strncmp(buffer, "RETR", 4) == 0) {
            char filename[BUFFER_SIZE];
            sscanf(buffer + 5, "%s", filename);

            if (fileExists(filename)) {
                // File exists, send the file
                FILE *file = fopen(filename, "rb");
                if (file) {
                    // Send file in chunks
                    size_t bytesRead;
                    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
                        send(new_socket, buffer, bytesRead, 0);
                    }
                    fclose(file);
                    printf("File %s sent successfully.\n", filename);
                } else {
                    snprintf(client_message, sizeof(client_message), "Failed to open file %s.\n", filename);
                    send(new_socket, client_message, strlen(client_message), 0);
                }
            } else {
                snprintf(client_message, sizeof(client_message), "File %s does not exist.\n", filename);
                send(new_socket, client_message, strlen(client_message), 0);
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
                send(new_socket, client_message, strlen(client_message), 0);
            } else {
                snprintf(client_message, sizeof(client_message), "Failed to list directory %s.\n", dirname);
                send(new_socket, client_message, strlen(client_message), 0);
            }
        }
    }
    close(new_socket);
    pthread_exit(NULL);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("FTP Server listening on port %d...\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        printf("Client connected from IP: %s, Port: %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        pthread_t client_thread;
        int *new_sock = malloc(sizeof(int));
        *new_sock = new_socket;
        pthread_create(&client_thread, NULL, handle_client, (void *)new_sock);
        pthread_detach(client_thread);
    }

    return 0;
}