#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <poll.h>
#include <vector>
#include <cstring>

#define PORT 8080
#define MAX_CLIENTS 100

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Binding the socket to the network address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    std::vector<pollfd> poll_fds;
    pollfd server_pollfd = {server_fd, POLLIN, 0};
    poll_fds.push_back(server_pollfd);

    while (true) {
        int poll_count = poll(poll_fds.data(), poll_fds.size(), -1);

        if (poll_count < 0) {
            perror("poll");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        for (size_t i = 0; i < poll_fds.size(); ++i) {
            if (poll_fds[i].revents & POLLIN) {
                if (poll_fds[i].fd == server_fd) {
                    // Accept new incoming connection
                    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                        perror("accept");
                        close(server_fd);
                        exit(EXIT_FAILURE);
                    }
                    pollfd client_pollfd = {new_socket, POLLIN, 0};
                    poll_fds.push_back(client_pollfd);
                    std::cout << "New connection accepted" << std::endl;
                } else {
                    // Handle data from an existing client
                    char buffer[1024] = {0};
                    int valread = read(poll_fds[i].fd, buffer, 1024);
                    if (valread == 0) {
                        // Client disconnected
                        close(poll_fds[i].fd);
                        poll_fds.erase(poll_fds.begin() + i);
                        std::cout << "Client disconnected" << std::endl;
                    } else {
                        // Echo the message back to the client
                        send(poll_fds[i].fd, buffer, valread, 0);
                    }
                }
            }
        }
    }

    close(server_fd);
    return 0;
}