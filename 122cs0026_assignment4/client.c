#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define KEY_SIZE 8

void decode_message(char *encoded_message, char *key, char *decoded_message) {
    int len = strlen(encoded_message);
    for (int i = 0; i < len; i++) {
        decoded_message[i] = (encoded_message[i] - key[i % KEY_SIZE] + 256) % 256;
    }
    decoded_message[len] = '\0';
}

void *receive_messages(void *arg) {
    int sock_fd = *(int *)arg;
    char buffer[BUFFER_SIZE + KEY_SIZE + 1];
    char encoded_message[BUFFER_SIZE];
    char key[KEY_SIZE + 1];
    char decoded_message[BUFFER_SIZE];

    while (1) {
        int bytes_received = recv(sock_fd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            break;
        }
        buffer[bytes_received] = '\0';

        char *delimiter = strchr(buffer, ':');
        if (delimiter) {
            int encoded_length = delimiter - buffer;
            strncpy(encoded_message, buffer, encoded_length);
            encoded_message[encoded_length] = '\0';
            strcpy(key, delimiter + 1);

            decode_message(encoded_message, key, decoded_message);
            printf("Received: %s\n", decoded_message);
        }
    }

    return NULL;
}

int main() {
    int sock_fd;
    struct sockaddr_in server_addr;
    pthread_t receive_thread;

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server\n");

    pthread_create(&receive_thread, NULL, receive_messages, &sock_fd);

    char message[BUFFER_SIZE];
    while (1) {
        fgets(message, BUFFER_SIZE, stdin);
        message[strcspn(message, "\n")] = 0;

        if (send(sock_fd, message, strlen(message), 0) < 0) {
            perror("Send failed");
            break;
        }

        if (strcmp(message, "exit") == 0) {
            break;
        }
    }

    close(sock_fd);
    pthread_join(receive_thread, NULL);

    return 0;
}
