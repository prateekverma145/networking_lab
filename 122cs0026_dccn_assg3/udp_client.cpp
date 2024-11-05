#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>

#define PORT 8080
#define BUFFER_SIZE 1024
//#define SERVER_IP "127.0.0.1"
void sendFile(int sockfd, const char* filename, struct sockaddr_in* servaddr) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cout << "Error opening file: " << filename << std::endl;
        return;
    }

    char buffer[BUFFER_SIZE];
    while (file.read(buffer, BUFFER_SIZE)) {
        sendto(sockfd, buffer, BUFFER_SIZE, MSG_CONFIRM, 
               (const struct sockaddr *)servaddr, sizeof(*servaddr));
    }
    if (file.gcount() > 0) {
        sendto(sockfd, buffer, file.gcount(), MSG_CONFIRM, 
               (const struct sockaddr *)servaddr, sizeof(*servaddr));
    }
    sendto(sockfd, "END", 3, MSG_CONFIRM, 
           (const struct sockaddr *)servaddr, sizeof(*servaddr));

    file.close();

    // Receive confirmation
    int n = recvfrom(sockfd, buffer, BUFFER_SIZE, MSG_WAITALL, 
                     (struct sockaddr *)servaddr, (socklen_t*)sizeof(*servaddr));
    buffer[n] = '\0';
    std::cout << "Server response: " << buffer << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <server_ip>" << std::endl;
        exit(EXIT_FAILURE);
    }

    const char* SERVER_IP = argv[1];
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in servaddr;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    while (true) {
        std::string command;
        std::cout << "Enter command (PWD, MKDIR,CWD, STOR, LIST, RETR, or EXIT): ";
        std::getline(std::cin, command);

        if (command == "EXIT") {
            break;
        }
 memset(buffer, 0, BUFFER_SIZE);
        if (command.substr(0, 5) == "STOR ") {
    std::string filename = command.substr(5);
    sendto(sockfd, command.c_str(), command.length(), MSG_CONFIRM, 
           (const struct sockaddr *)&servaddr, sizeof(servaddr));
    sendFile(sockfd, filename.c_str(), &servaddr);
    continue;
}

        sendto(sockfd, command.c_str(), command.length(), MSG_CONFIRM, 
               (const struct sockaddr *)&servaddr, sizeof(servaddr));
         memset(buffer, 0, BUFFER_SIZE);
        int n;
        unsigned int len = sizeof(servaddr);
        n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, 
                     (struct sockaddr *)&servaddr, &len);
        buffer[n] = '\0';

        if (command.substr(0, 5) == "RETR ") {
            std::string filename = command.substr(5);
            std::ofstream file(filename, std::ios::binary);
            if (file) {
                file.write(buffer + 13, n - 13);  // Skip "File content:\n"
                file.close();
                std::cout << "File retrieved and saved as: " << filename << std::endl;
            } else {
                std::cout << "Error saving retrieved file: " << filename << std::endl;
            }
        } else {
            std::cout << "Server response: " << buffer << std::endl;
        }
        memset(buffer, 0, BUFFER_SIZE);

    }

    close(sockfd);
    return 0;
}
