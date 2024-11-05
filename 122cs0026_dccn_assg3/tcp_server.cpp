#include <iostream>
#include <thread>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define CONTROL_PORT 21
#define DATA_PORT 20

namespace fs = std::filesystem;

class FTPServer {
private:
    int control_socket, data_socket;
    struct sockaddr_in control_address, data_address;
    
    int create_socket(int port) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }
        
        int opt = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            perror("setsockopt failed");
            exit(EXIT_FAILURE);
        }
        
        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);
        
        if (bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("Bind failed");
            exit(EXIT_FAILURE);
        }
        
        if (listen(sock, 3) < 0) {
            perror("Listen failed");
            exit(EXIT_FAILURE);
        }
        
        return sock;
    }
    
    void handle_client(int client_control_socket) {
        char buffer[1024] = {0};
        send(client_control_socket, "220 Welcome to FTP server\r\n", 28, 0);
        
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            int valread = read(client_control_socket, buffer, 1024);
            if (valread <= 0) {
                std::cout << "Client disconnected" << std::endl;
                break;
            }
            
            std::string command(buffer);
            std::cout << "Received command: " << command;
            
            if (command.rfind("USER ", 0) == 0) {
                send(client_control_socket, "331 User name okay, need password\r\n", 36, 0);
            }
            else if (command.rfind("PASS ", 0) == 0) {
                send(client_control_socket, "230 User logged in\r\n", 21, 0);
            }
            else if (command.rfind("PWD", 0) == 0) {
                std::string cwd = fs::current_path().string();
                std::string response = "257 \"" + cwd + "\" is the current directory\r\n";
                send(client_control_socket, response.c_str(), response.length(), 0);
            }
            else if (command.rfind("CWD ", 0) == 0) {
                std::string dir = command.substr(4);
                dir.erase(dir.find_last_not_of(" \n\r\t") + 1);
                try {
                    fs::current_path(dir);
                    send(client_control_socket, "250 Directory changed successfully\r\n", 36, 0);
                } catch (const fs::filesystem_error& e) {
                    send(client_control_socket, "550 Failed to change directory\r\n", 32, 0);
                }
            }
            else if (command.rfind("LIST", 0) == 0) {
                send(client_control_socket, "150 Opening data connection for directory listing\r\n", 52, 0);
                
                int data_client_socket = accept(data_socket, NULL, NULL);
                std::string listing;
                for (const auto & entry : fs::directory_iterator(fs::current_path())) {
                    listing += entry.path().filename().string() + "\r\n";
                }
                send(data_client_socket, listing.c_str(), listing.length(), 0);
                close(data_client_socket);
                
                send(client_control_socket, "226 Directory listing completed\r\n", 33, 0);
            }
            else if (command.rfind("RETR ", 0) == 0) {
                std::string filename = command.substr(5);
                filename.erase(filename.find_last_not_of(" \n\r\t") + 1);
                std::ifstream file(filename, std::ios::binary);
                
                if (file) {
                    send(client_control_socket, "150 Opening data connection for file transfer\r\n", 48, 0);
                    
                    int data_client_socket = accept(data_socket, NULL, NULL);
                    char file_buffer[1024];
                    while (file.read(file_buffer, sizeof(file_buffer)).gcount() > 0) {
                        send(data_client_socket, file_buffer, file.gcount(), 0);
                    }
                    file.close();
                    close(data_client_socket);
                    
                    send(client_control_socket, "226 File transfer completed\r\n", 29, 0);
                } else {
                    send(client_control_socket, "550 File not found\r\n", 21, 0);
                }
            }
            else if (command.rfind("STOR ", 0) == 0) {
                std::string filename = command.substr(5);
                filename.erase(filename.find_last_not_of(" \n\r\t") + 1);
                std::ofstream file(filename, std::ios::binary);
                
                if (file) {
                    send(client_control_socket, "150 Opening data connection for file transfer\r\n", 48, 0);
                    
                    int data_client_socket = accept(data_socket, NULL, NULL);
                    char file_buffer[1024];
                    int bytes_read;
                    while ((bytes_read = recv(data_client_socket, file_buffer, sizeof(file_buffer), 0)) > 0) {
                        file.write(file_buffer, bytes_read);
                    }
                    file.close();
                    close(data_client_socket);
                    
                    send(client_control_socket, "226 File transfer completed\r\n", 29, 0);
                } else {
                    send(client_control_socket, "550 Failed to create file\r\n", 28, 0);
                }
            }
            else if (command.rfind("QUIT", 0) == 0) {
                send(client_control_socket, "221 Goodbye\r\n", 14, 0);
                break;
            }
             else if (command.rfind("MKD ", 0) == 0) {
                std::string dir_name = command.substr(4);
                dir_name.erase(dir_name.find_last_not_of(" \n\r\t") + 1);
                try {
                    if (fs::create_directory(dir_name)) {
                        std::string response = "257 \"" + dir_name + "\" directory created\r\n";
                        send(client_control_socket, response.c_str(), response.length(), 0);
                    } else {
                        send(client_control_socket, "550 Failed to create directory\r\n", 32, 0);
                    }
                } catch (const fs::filesystem_error& e) {
                    send(client_control_socket, "550 Failed to create directory\r\n", 32, 0);
                }
            }
            else if (command.rfind("QUIT", 0) == 0) {
                send(client_control_socket, "221 Goodbye\r\n", 14, 0);
                break;
            }
            else {
                send(client_control_socket, "500 Unknown command\r\n", 22, 0);
            }
        }
        
        close(client_control_socket);
    }

public:
    FTPServer() {
        control_socket = create_socket(CONTROL_PORT);
        data_socket = create_socket(DATA_PORT);
        std::cout << "FTP Server is running on port " << CONTROL_PORT << std::endl;
    }
    
    void run() {
        while (true) {
            int client_control_socket = accept(control_socket, NULL, NULL);
            if (client_control_socket < 0) {
                perror("Accept failed");
                continue;
            }
            
            std::thread client_thread(&FTPServer::handle_client, this, client_control_socket);
            client_thread.detach();
        }
    }
};

int main() {
    FTPServer server;
    server.run();
    return 0;
}
