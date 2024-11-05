#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define KEY_SIZE 6

int clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void generate_key(char *key) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < KEY_SIZE - 1; i++) {
        int index = rand() % (sizeof(charset) - 1);
        key[i] = charset[index];
    }
    key[KEY_SIZE - 1] = '\0';
}

void encode_message(char *message, char *key, char *encoded_message) {
    int len = strlen(message);
    for (int i = 0; i < len; i++) {
        encoded_message[i] = (message[i] + key[i % KEY_SIZE]) % 256;
    }
    encoded_message[len] = '\0';
}

void decode_message(char *encoded_message, char *key, char *decoded_message) {
    int len = strlen(encoded_message);
    for (int i = 0; i < len; i++) {
        decoded_message[i] = (encoded_message[i] - key[i % KEY_SIZE] + 256) % 256;
    }
    decoded_message[len] = '\0';
}

void broadcast_message(char *message, char *key, int sender_fd) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i] != sender_fd) {
            char buffer[BUFFER_SIZE + KEY_SIZE + 1];
            sprintf(buffer, "%s:%s", message, key);
            send(clients[i], buffer, strlen(buffer), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    char buffer[BUFFER_SIZE];
    char key[KEY_SIZE];
    char encoded_message[BUFFER_SIZE];

    while (1) {
        int bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }
        buffer[bytes_received] = '\0';

        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        generate_key(key);
        encode_message(buffer, key, encoded_message);
        
        // Print the original message, encoded message, and key on the server screen
        printf("Original message: %s\n", buffer);
        printf("Encoded message: ");
        for (int i = 0; i < strlen(encoded_message); i++) {
            printf("%02X", (unsigned char)encoded_message[i]);
        }
        printf("\n");
        printf("Key: %s\n", key);
        printf("--------------------\n");
        
        broadcast_message(encoded_message, key, client_fd);
    }

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i] == client_fd) {
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    close(client_fd);
    free(arg);
    return NULL;
}

int main() {
    int server_fd, *client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);
    pthread_t tid;

    srand(time(NULL));

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port 5000\n");

    while (1) {
        client_fd = malloc(sizeof(int));
        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
        if (*client_fd < 0) {
            perror("Accept failed");
            free(client_fd);
            continue;
        }

        pthread_mutex_lock(&clients_mutex);
        if (client_count < MAX_CLIENTS) {
            clients[client_count++] = *client_fd;
            pthread_create(&tid, NULL, handle_client, client_fd);
            printf("New client connected\n");
        } else {
            printf("Max clients reached. Connection rejected.\n");
            close(*client_fd);
            free(client_fd);
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    close(server_fd);
    return 0;
}
