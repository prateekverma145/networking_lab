#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>

#define CONTROL_PORT 21
#define DATA_PORT 20

class FTPClient {
private:
    int control_socket;
    struct sockaddr_in server_addr;
    
    int create_data_connection() {
        int data_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (data_socket < 0) {
            std::cerr << "Data socket creation error" << std::endl;
            return -1;
        }
        
        server_addr.sin_port = htons(DATA_PORT);
        
        if (connect(data_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Data connection failed" << std::endl;
            return -1;
        }
        
        return data_socket;
    }
    
    void send_command(const std::string& command) {
        send(control_socket, (command + "\r\n").c_str(), command.length() + 2, 0);
    }
    
    std::string receive_response() {
        char buffer[1024] = {0};
        read(control_socket, buffer, 1024);
        return std::string(buffer);
    }

public:
    FTPClient(const std::string& server_ip) {
        control_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (control_socket < 0) {
            std::cerr << "Socket creation error" << std::endl;
            exit(EXIT_FAILURE);
        }
        
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(CONTROL_PORT);
        
        if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address/ Address not supported" << std::endl;
            exit(EXIT_FAILURE);
        }
        
        if (connect(control_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Connection Failed" << std::endl;
            exit(EXIT_FAILURE);
        }
        
        std::cout << receive_response();
    }
    
    void login(const std::string& username, const std::string& password) {
        send_command("USER " + username);
        std::cout << receive_response();
        send_command("PASS " + password);
        std::cout << receive_response();
    }
    
    void pwd() {
        send_command("PWD");
        std::cout << receive_response();
    }
    
    void cwd(const std::string& directory) {
        send_command("CWD " + directory);
        std::cout << receive_response();
    }
    
    void list() {
        send_command("LIST");
        std::cout << receive_response();
        
        int data_socket = create_data_connection();
        char buffer[1024] = {0};
        int bytes_read;
        while ((bytes_read = read(data_socket, buffer, 1024)) > 0) {
            std::cout.write(buffer, bytes_read);
        }
        close(data_socket);
        
        std::cout << receive_response();
    }
    
    void retr(const std::string& filename) {
        send_command("RETR " + filename);
        std::cout << receive_response();
        
        int data_socket = create_data_connection();
        std::ofstream file(filename, std::ios::binary);
        char buffer[1024] = {0};
        int bytes_read;
        while ((bytes_read = read(data_socket, buffer, 1024)) > 0) {
            file.write(buffer, bytes_read);
        }
        file.close();
        close(data_socket);
        
        std::cout << receive_response();
    }
    
    void stor(const std::string& filename) {
        send_command("STOR " + filename);
        std::cout << receive_response();
        
        int data_socket = create_data_connection();
        std::ifstream file(filename, std::ios::binary);
        char buffer[1024] = {0};
        while (file.read(buffer, sizeof(buffer)).gcount() > 0) {
            send(data_socket, buffer, file.gcount(), 0);
        }
        file.close();
        close(data_socket);
        
        std::cout << receive_response();
    }
    
    void quit() {
        send_command("QUIT");
        std::cout << receive_response();
        close(control_socket);
    }
    void mkd(const std::string& directory) {
    send_command("MKD " + directory);
    std::cout << receive_response();
}
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <server_ip>" << std::endl;
        exit(EXIT_FAILURE);
    }

    const char* SERVER_IP = argv[1];
    FTPClient client(SERVER_IP);
    client.login("username", "password");
    
    while (true) {
        std::string command;
        std::cout << "Enter FTP command (PWD, CWD,MKD, LIST, RETR, STOR, QUIT): ";
        std::getline(std::cin, command);
        
        if (command == "PWD") {
            client.pwd();
        } else if (command.substr(0, 3) == "CWD") {
            client.cwd(command.substr(4));
        } else if (command == "LIST") {
            client.list();
        } else if (command.substr(0, 4) == "RETR") {
            client.retr(command.substr(5));
        } else if (command.substr(0, 4) == "STOR") {
            client.stor(command.substr(5));
        }else if (command.substr(0, 3) == "MKD") {
            client.mkd(command.substr(4));
        }
        else if (command == "QUIT") {
            client.quit();
            break;
        } else {
            std::cout << "Unknown command" << std::endl;
        }
    }
    
    return 0;
}
