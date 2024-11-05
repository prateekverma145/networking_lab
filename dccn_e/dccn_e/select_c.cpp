#include <iostream>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 12345
#define BUFFER_SIZE 1024


int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    std::string message;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address or Address not supported" << std::endl;
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return -1;
    }

    while (true) {
        std::cout << "Enter message to send (or 'exit' to quit): ";
        std::getline(std::cin, message);

        if (message == "exit") {
            break;
        }

        send(sock, message.c_str(), message.length(), 0);
        std::cout << "Message sent: " << message << std::endl;

        int valread = read(sock, buffer, BUFFER_SIZE);
        std::cout << "Server: " << buffer << std::endl;
        memset(buffer, 0, BUFFER_SIZE); // Clear buffer for next message
    }

    close(sock);
    return 0;
}
