#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <ctime>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048
#define KEY_SIZE 5

std::vector<int> clients;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void generate_key(char *key) {
    const char charset[] = "0123456789";
    for (int i = 0; i < KEY_SIZE - 1; i++) {
        int index = rand() % (sizeof(charset) - 1);
        key[i] = charset[index];
    }
    key[KEY_SIZE - 1] = '\0';
}

void encode(const char *message, const char *key, char *encoded_message) {
    int len = strlen(message);
    for (int i = 0; i < len; i++) {
        encoded_message[i] = (message[i] + key[i % KEY_SIZE]) % 128;
    }
    encoded_message[len] = '\0';
}

void decode(const char *encoded_message, const char *key, char *decoded_message) {
    int len = strlen(encoded_message);
    for (int i = 0; i < len; i++) {
        decoded_message[i] = (encoded_message[i] - key[i % KEY_SIZE] + 128) % 128;
    }
    decoded_message[len] = '\0';
}

void broadcast_message(const char *message, const char *key, int sender_fd) {
    pthread_mutex_lock(&clients_mutex);
    for (int client_fd : clients) {
        if (client_fd != sender_fd) {
            char buffer[BUFFER_SIZE + KEY_SIZE + 1];
            sprintf(buffer, "%s:%s", message, key);
            send(client_fd, buffer, strlen(buffer), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    char buffer[BUFFER_SIZE];
    char key[KEY_SIZE];
    char encoded_message[BUFFER_SIZE];

    while (true) {
        int bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }
        buffer[bytes_received] = '\0';

        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        generate_key(key);
        encode(buffer, key, encoded_message);
        
        std::cout << "Original message: " << buffer << std::endl;
        std::cout << "Encoded message: ";
        for (int i = 0; i < strlen(encoded_message); i++) {
            std::cout << std::hex << (unsigned char)encoded_message[i];
        }
        std::cout << std::endl;
        std::cout << "Key: " << key << std::endl;
        std::cout << "--------------------" << std::endl;
        
        broadcast_message(encoded_message, key, client_fd);
    }

    pthread_mutex_lock(&clients_mutex);
    clients.erase(std::remove(clients.begin(), clients.end(), client_fd), clients.end());
    pthread_mutex_unlock(&clients_mutex);

    close(client_fd);
    free(arg);
    return nullptr;
}

int main() {
    int server_fd, *client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);
    pthread_t tid;

    srand(time(nullptr));

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

    std::cout << "Server listening on port 8080" << std::endl;

    while (true) {
        client_fd = new int;
        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
        if (*client_fd < 0) {
            perror("Accept failed");
            delete client_fd;
            continue;
        }

        pthread_mutex_lock(&clients_mutex);
        if (clients.size() < MAX_CLIENTS) {
            clients.push_back(*client_fd);
            pthread_create(&tid, nullptr, handle_client, client_fd);
            std::cout << "New client connected" << std::endl;
        } else {
            std::cout << "Max clients reached. Connection rejected." << std::endl;
            close(*client_fd);
            delete client_fd;
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    close(server_fd);
    return 0;
}