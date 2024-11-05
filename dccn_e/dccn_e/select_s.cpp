#include <iostream>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 12345
#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024

int main() {
    int listen_sd, new_sd, client_sd[MAX_CLIENTS], max_sd, sd, activity, valread, i;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];
    fd_set master_set, working_set;

    for (i = 0; i < MAX_CLIENTS; i++) {
        client_sd[i] = 0;
    }

    listen_sd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sd == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(listen_sd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(listen_sd);
        exit(EXIT_FAILURE);
    }

    if (listen(listen_sd, 3) < 0) {
        perror("Listen failed");
        close(listen_sd);
        exit(EXIT_FAILURE);
    }

    FD_ZERO(&master_set);
    FD_SET(listen_sd, &master_set);
    max_sd = listen_sd;

    std::cout << "Server listening on PORT " << PORT << std::endl;

    while (true) {
        working_set = master_set;

        activity = select(max_sd + 1, &working_set, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            perror("select error");
        }

        if (FD_ISSET(listen_sd, &working_set)) {
            new_sd = accept(listen_sd, (struct sockaddr*)&address, &addrlen);
            if (new_sd < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            std::cout << "New connection: socket fd is " << new_sd
                      << ", IP: " << inet_ntoa(address.sin_addr)
                      << ", Port: " << ntohs(address.sin_port) << std::endl;

            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_sd[i] == 0) {
                    client_sd[i] = new_sd;
                    std::cout << "Added to list of sockets at index " << i << std::endl;
                    break;
                }
            }

            FD_SET(new_sd, &master_set);
            if (new_sd > max_sd) {
                max_sd = new_sd;
            }
        }

        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sd[i];

            if (FD_ISSET(sd, &working_set)) {
                while (true) {
                    valread = read(sd, buffer, BUFFER_SIZE - 1);
                    if (valread <= 0) {
                        if (valread == 0) {
                            // Client has disconnected
                            getpeername(sd, (struct sockaddr*)&address, &addrlen);
                            std::cout << "Client disconnected: IP " << inet_ntoa(address.sin_addr)
                                      << ", Port " << ntohs(address.sin_port) << std::endl;
                        } else {
                            perror("Read error");
                        }
                        close(sd);
                        client_sd[i] = 0;
                        FD_CLR(sd, &master_set);
                        break;
                    } else {
                        buffer[valread] = '\0';
                        std::cout << "Received: " << buffer << std::endl;
                        send(sd, buffer, valread, 0);
                    }

                    if (valread < BUFFER_SIZE - 1) {
                        break;
                    }
                }
            }
        }
    }

    return 0;
}
