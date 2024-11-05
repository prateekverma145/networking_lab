#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    struct pollfd fds[2];

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return -1;
    }

    // Set up poll
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    while (true) {
        int poll_count = poll(fds, 2, -1);

        if (poll_count == -1) {
            std::cerr << "Poll error" << std::endl;
            break;
        }

        // Check for data from server
        if (fds[0].revents & POLLIN) {
            int bytes_received = read(sockfd, buffer, BUFFER_SIZE);
            if (bytes_received <= 0) {
                std::cerr << "Server disconnected" << std::endl;
                break;
            }
            buffer[bytes_received] = '\0';
            std::cout << "Server: " << buffer << std::endl;
        }

        // Check for user input
        if (fds[1].revents & POLLIN) {
            std::cin.getline(buffer, BUFFER_SIZE);
            send(sockfd, buffer, strlen(buffer), 0);
        }
    }

    close(sockfd);
    return 0;
}