// TCP Server
#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
void TCPServer() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Socket creation error" << std::endl;
        return;
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::cerr << "Setsockopt error" << std::endl;
        return;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return;
    }
    
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen error" << std::endl;
        return;
    }
    
    std::cout << "TCP Server listening on port 8080..." << std::endl;
    
    while(true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            std::cerr << "Accept error" << std::endl;
            continue;
        }
        
        read(new_socket, buffer, 1024);
        std::cout << "Message received: " << buffer << std::endl;
        
        send(new_socket, buffer, strlen(buffer), 0);
        
        close(new_socket);
    }
}

void TCPClient(const char* message) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address" << std::endl;
        return;
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return;
    }
    
    send(sock, message, strlen(message), 0);
    std::cout << "Message sent: " << message << std::endl;
    
    read(sock, buffer, 1024);
    std::cout << "Server response: " << buffer << std::endl;
    
    close(sock);
}

void UDPServer() {
    int server_fd;
    char buffer[1024] = {0};
    struct sockaddr_in address, client_addr;
    
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8081);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return;
    }
    
    std::cout << "UDP Server listening on port 8081..." << std::endl;
    
    while(true) {
        socklen_t len = sizeof(client_addr);
        int n = recvfrom(server_fd, buffer, 1024, MSG_WAITALL, 
                        (struct sockaddr *)&client_addr, &len);
        buffer[n] = '\0';
        
        std::cout << "Message received: " << buffer << std::endl;
        
        sendto(server_fd, buffer, strlen(buffer), MSG_CONFIRM,
               (struct sockaddr *)&client_addr, len);
    }
}

void UDPClient(const char* message) {
    int sock;
    char buffer[1024] = {0};
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8081);
    
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address" << std::endl;
        return;
    }
    
    sendto(sock, message, strlen(message), MSG_CONFIRM,
           (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    std::cout << "Message sent: " << message << std::endl;
    
    socklen_t len = sizeof(serv_addr);
    int n = recvfrom(sock, buffer, 1024, MSG_WAITALL,
                     (struct sockaddr *)&serv_addr, &len);
    buffer[n] = '\0';
    std::cout << "Server response: " << buffer << std::endl;
    
    close(sock);
}

int main() {
    // Example usage:
    // For TCP:
    // In one terminal: ./program server tcp
    // In another terminal: ./program client tcp "Hello TCP!"
    
    // For UDP:
    // In one terminal: ./program server udp
    // In another terminal: ./program client udp "Hello UDP!"
    
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <server/client> <tcp/udp> [message]" << std::endl;
        return 1;
    }
    
    std::string mode = argv[1];
    std::string protocol = argv[2];
    
    if (mode == "server") {
        if (protocol == "tcp") {
            TCPServer();
        } else if (protocol == "udp") {
            UDPServer();
        }
    } else if (mode == "client") {
        if (argc < 4) {
            std::cout << "Please provide a message for the client" << std::endl;
            return 1;
        }
        if (protocol == "tcp") {
            TCPClient(argv[3]);
        } else if (protocol == "udp") {
            UDPClient(argv[3]);
        }
    }
    
    return 0;
}