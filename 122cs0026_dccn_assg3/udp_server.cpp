#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <vector>

#define PORT 8080
#define BUFFER_SIZE 1024

void handlePWD(char* response) {
    char cwd[BUFFER_SIZE];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        snprintf(response, BUFFER_SIZE, "Current working directory: %s", cwd);
    } else {
        snprintf(response, BUFFER_SIZE, "Error getting current working directory");
    }
}

void handleCWD(const char* path, char* response) {
    if (chdir(path) == 0) {
        snprintf(response, BUFFER_SIZE, "Changed directory to: %s", path);
    } else {
        snprintf(response, BUFFER_SIZE, "Error changing directory");
    }
}

void handleSTOR(int sockfd, const char* filename, struct sockaddr_in* cliaddr, socklen_t clilen) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        sendto(sockfd, "Error opening file for writing", 30, MSG_CONFIRM, 
               (const struct sockaddr *)cliaddr, clilen);
        return;
    }

    char buffer[BUFFER_SIZE];
    int n;
    while (true) {
        n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, 
                     (struct sockaddr *)cliaddr, &clilen);
        if (n <= 0) break;
        if (strncmp(buffer, "END", 3) == 0) break;
        file.write(buffer, n);
    }
    file.close();
    sendto(sockfd, "File stored successfully", 24, MSG_CONFIRM, 
           (const struct sockaddr *)cliaddr, clilen);
}



void handleLIST(char* response) {
    DIR *dir;
    struct dirent *ent;
    std::string list;

    if ((dir = opendir(".")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            list += ent->d_name;
            list += "\n";
        }
        closedir(dir);
        snprintf(response, BUFFER_SIZE, "Directory listing:\n%s", list.c_str());
    } else {
        snprintf(response, BUFFER_SIZE, "Error listing directory");
    }
}

void handleRETR(const char* filename, char* response, int& response_len) {
    std::ifstream file(filename, std::ios::binary);
    if (file.is_open()) {
        file.seekg(0, std::ios::end);
        int file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (file_size > BUFFER_SIZE - 100) {  // Leave some space for the header
            snprintf(response, BUFFER_SIZE, "Error: File too large to send");
            response_len = strlen(response);
        } else {
            snprintf(response, BUFFER_SIZE, "File content:\n");
            int header_len = strlen(response);
            file.read(response + header_len, file_size);
            response_len = header_len + file_size;
        }
        file.close();
    } else {
        snprintf(response, BUFFER_SIZE, "Error opening file: %s", filename);
        response_len = strlen(response);
    }
}
void handleMKDIR(const char* dirname, char* response) {
    if (mkdir(dirname, 0777) == 0) {
        snprintf(response, BUFFER_SIZE, "Directory created successfully: %s", dirname);
    } else {
        snprintf(response, BUFFER_SIZE, "Error creating directory %s: %s", dirname, strerror(errno));
    }
}

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    struct sockaddr_in servaddr, cliaddr;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (true) {
        int len, n;
        len = sizeof(cliaddr);

        n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, 
                    (struct sockaddr *)&cliaddr, (socklen_t*)&len);
        buffer[n] = '\0';

        printf("Client : %s\n", buffer);

        int response_len = 0;
        memset(response, 0, BUFFER_SIZE);
        if (strncmp(buffer, "PWD", 3) == 0) {
            handlePWD(response);
        } else if (strncmp(buffer, "CWD ", 4) == 0) {
            handleCWD(buffer + 4, response);
        } else if (strncmp(buffer, "STOR ", 5) == 0) {
    		handleSTOR(sockfd, buffer + 5, &cliaddr, len);
	} else if (strncmp(buffer, "LIST", 4) == 0) {
            handleLIST(response);
        } else if (strncmp(buffer, "RETR ", 5) == 0) {
            handleRETR(buffer + 5, response, response_len);
        } else if (strncmp(buffer, "MKDIR ", 6) == 0) {
            handleMKDIR(buffer + 6, response);
        }
        else {
            snprintf(response, BUFFER_SIZE, "Unknown command");
        }

        if (response_len == 0) {
            response_len = strlen(response);
        }

        sendto(sockfd, response, response_len, MSG_CONFIRM, 
               (const struct sockaddr *)&cliaddr, len);
               memset(buffer, 0, BUFFER_SIZE);
    }

    return 0;
}
