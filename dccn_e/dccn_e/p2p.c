#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define MAX_FILES 100
#define MAX_FILENAME 256
#define MAX_PEERS 50
#define SHARE_DIR "./shared"

void download_file(const char* peer_ip, int peer_port, const char* filename);

typedef struct {
    char filename[MAX_FILENAME];
    char ip[INET_ADDRSTRLEN];
    int port;
} FileInfo;

typedef struct {
    FileInfo files[MAX_FILES];
    int file_count;
} Tracker;

typedef struct {
    char ip[INET_ADDRSTRLEN];
    int port;
} PeerInfo;

typedef struct {
    char ip[INET_ADDRSTRLEN];
    int port;
    char files[MAX_FILES][MAX_FILENAME];
    int file_count;
    PeerInfo known_peers[MAX_PEERS];
    int peer_count;
} Peer;

Tracker tracker;
Peer peer;

void init_tracker() {
    tracker.file_count = 0;
}

void init_peer(const char* ip, int port) {
    strcpy(peer.ip, ip);
    peer.port = port;
    peer.file_count = 0;
    peer.peer_count = 0;
    mkdir(SHARE_DIR, 0777);
}

void add_file_to_tracker(const char* filename, const char* ip, int port) {
    if (tracker.file_count < MAX_FILES) {
        strcpy(tracker.files[tracker.file_count].filename, filename);
        strcpy(tracker.files[tracker.file_count].ip, ip);
        tracker.files[tracker.file_count].port = port;
        tracker.file_count++;
    }
}

void add_file_to_peer(const char* filename) {
    if (peer.file_count < MAX_FILES) {
        strcpy(peer.files[peer.file_count], filename);
        peer.file_count++;
    }
}

FileInfo* search_file(const char* filename) {
    for (int i = 0; i < tracker.file_count; i++) {
        if (strcmp(tracker.files[i].filename, filename) == 0) {
            return &tracker.files[i];
        }
    }
    return NULL;
}

void* handle_tracker_connection(void* socket_desc) {
    int client_sock = *(int*)socket_desc;
    char buffer[BUFFER_SIZE];
    int read_size;

    while ((read_size = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';

        if (strncmp(buffer, "REGISTER", 8) == 0) {
            char filename[MAX_FILENAME];
            char ip[INET_ADDRSTRLEN];
            int port;
            sscanf(buffer, "REGISTER %s %s %d", filename, ip, &port);
            add_file_to_tracker(filename, ip, port);
            send(client_sock, "OK", 2, 0);
        } else if (strncmp(buffer, "SEARCH", 6) == 0) {
            char filename[MAX_FILENAME];
            sscanf(buffer, "SEARCH %s", filename);
            FileInfo* file = search_file(filename);
            if (file) {
                char response[BUFFER_SIZE];
                snprintf(response, BUFFER_SIZE, "%s %d", file->ip, file->port);
                send(client_sock, response, strlen(response), 0);
            } else {
                send(client_sock, "NOT FOUND", 9, 0);
            }
        } else if (strncmp(buffer, "GETPEERS", 8) == 0) {
            char response[BUFFER_SIZE] = "";
            for (int i = 0; i < tracker.file_count; i++) {
                char peer_info[50];
                snprintf(peer_info, 50, "%s %d,", tracker.files[i].ip, tracker.files[i].port);
                strcat(response, peer_info);
            }
            send(client_sock, response, strlen(response), 0);
        }
    }

    free(socket_desc);
    return NULL;
}

void start_tracker(int port) {
    int socket_desc, client_sock, c, *new_sock;
    struct sockaddr_in server, client;

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create socket");
        return;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind failed");
        return;
    }

    listen(socket_desc, 3);

    c = sizeof(struct sockaddr_in);
    while ((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c))) {
        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = client_sock;

        if (pthread_create(&sniffer_thread, NULL, handle_tracker_connection, (void*) new_sock) < 0) {
            perror("could not create thread");
            return;
        }
    }

    if (client_sock < 0) {
        perror("accept failed");
        return;
    }
}

void register_with_tracker(const char* tracker_ip, int tracker_port, const char* filename) {
    int sock;
    struct sockaddr_in server;
    char message[BUFFER_SIZE], server_reply[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Could not create socket");
        return;
    }

    server.sin_addr.s_addr = inet_addr(tracker_ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(tracker_port);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect failed");
        return;
    }

    snprintf(message, BUFFER_SIZE, "REGISTER %s %s %d", filename, peer.ip, peer.port);
    if (send(sock, message, strlen(message), 0) < 0) {
        puts("Send failed");
        return;
    }

    if (recv(sock, server_reply, BUFFER_SIZE, 0) < 0) {
        puts("recv failed");
    } else {
        puts("File registered with tracker");
    }

    close(sock);
}

void search_and_download(const char* tracker_ip, int tracker_port, const char* filename) {

    int sock;
    struct sockaddr_in server;
    char message[BUFFER_SIZE], server_reply[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Could not create socket");
        return;
    }

    server.sin_addr.s_addr = inet_addr(tracker_ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(tracker_port);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect failed");
        return;
    }

    snprintf(message, BUFFER_SIZE, "SEARCH %s", filename);
    if (send(sock, message, strlen(message), 0) < 0) {
        puts("Send failed");
        return;
    }

    if (recv(sock, server_reply, BUFFER_SIZE, 0) < 0) {
        puts("recv failed");
    } else {
        if (strcmp(server_reply, "NOT FOUND") == 0) {
            puts("File not found");
        } else {
            char peer_ip[INET_ADDRSTRLEN];
            int peer_port;
            sscanf(server_reply, "%s %d", peer_ip, &peer_port);
            printf("File found at %s:%d\n", peer_ip, peer_port);
            download_file(peer_ip, peer_port, filename);
        }
    }

    close(sock);
}

void download_file(const char* peer_ip, int peer_port, const char* filename) {
    int sock;
    struct sockaddr_in server;
    char message[BUFFER_SIZE], file_path[MAX_FILENAME + 20];
    FILE *file;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Could not create socket");
        return;
    }

    server.sin_addr.s_addr = inet_addr(peer_ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(peer_port);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect failed");
        return;
    }

    snprintf(message, BUFFER_SIZE, "GET %s", filename);
    if (send(sock, message, strlen(message), 0) < 0) {
        puts("Send failed");
        return;
    }

    snprintf(file_path, sizeof(file_path), "%s/%s", SHARE_DIR, filename);
    file = fopen(file_path, "wb");
    if (file == NULL) {
        printf("Failed to open file for writing");
        return;
    }

    int bytes_received;
    while ((bytes_received = recv(sock, message, BUFFER_SIZE, 0)) > 0) {
        fwrite(message, 1, bytes_received, file);
    }

    fclose(file);
    close(sock);
    printf("File downloaded successfully\n");
    add_file_to_peer(filename);
}

void* handle_peer_connection(void* socket_desc) {
    int client_sock = *(int*)socket_desc;
    char buffer[BUFFER_SIZE];
    int read_size;

    if ((read_size = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';

        if (strncmp(buffer, "GET", 3) == 0) {
            char filename[MAX_FILENAME];
            sscanf(buffer, "GET %s", filename);
            char file_path[MAX_FILENAME + 20];
            snprintf(file_path, sizeof(file_path), "%s/%s", SHARE_DIR, filename);
            
            FILE *file = fopen(file_path, "rb");
            if (file == NULL) {
                send(client_sock, "File not found", 14, 0);
            } else {
                int bytes_read;
                while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
                    send(client_sock, buffer, bytes_read, 0);
                }
                fclose(file);
            }
        }
    }

    free(socket_desc);
    close(client_sock);
    return NULL;
}

void* start_peer_server(void* arg) {
    int socket_desc, client_sock, c, *new_sock;
    struct sockaddr_in server, client;

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create socket");
        return NULL;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(peer.port);

    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind failed");
        return NULL;
    }

    listen(socket_desc, 3);

    c = sizeof(struct sockaddr_in);
    while ((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c))) {
        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = client_sock;

        if (pthread_create(&sniffer_thread, NULL, handle_peer_connection, (void*) new_sock) < 0) {
            perror("could not create thread");
            return NULL;
        }
    }

    if (client_sock < 0) {
        perror("accept failed");
        return NULL;
    }

    return NULL;
}

void discover_peers(const char* tracker_ip, int tracker_port) {
    int sock;
    struct sockaddr_in server;
    char message[BUFFER_SIZE], server_reply[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Could not create socket");
        return;
    }

    server.sin_addr.s_addr = inet_addr(tracker_ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(tracker_port);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect failed");
        return;
    }

    strcpy(message, "GETPEERS");
    if (send(sock, message, strlen(message), 0) < 0) {
        puts("Send failed");
        return;
    }

    if (recv(sock, server_reply, BUFFER_SIZE, 0) < 0) {
        puts("recv failed");
    } else {
        char *token = strtok(server_reply, ",");
        while (token != NULL && peer.peer_count < MAX_PEERS) {
            char ip[INET_ADDRSTRLEN];
            int port;
            sscanf(token, "%s %d", ip, &port);
            if (strcmp(ip, peer.ip) != 0 || port != peer.port) {
                strcpy(peer.known_peers[peer.peer_count].ip, ip);
                peer.known_peers[peer.peer_count].port = port;
                peer.peer_count++;
            }
            token = strtok(NULL, ",");
        }
        printf("Discovered %d peers\n", peer.peer_count);
    }

    close(sock);
}

void list_shared_files() {
    DIR *d;
    struct dirent *dir;
    d = opendir(SHARE_DIR);
    if (d) {
        printf("Shared files:\n");
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) {
                printf("%s\n", dir->d_name);
            }
        }
        closedir(d);
    }
}
void print_menu() {
    printf("\n--- P2P File Sharing Application ---\n");
    printf("1. Register a file\n");
    printf("2. Search and download a file\n");
    printf("3. List shared files\n");
    printf("4. Discover peers\n");
    printf("5. Exit\n");
    printf("Enter your choice: ");
}

void register_file() {
    char filename[MAX_FILENAME];
    char filepath[MAX_FILENAME + 20];
    printf("Enter the name of the file to register: ");
    scanf("%s", filename);
    snprintf(filepath, sizeof(filepath), "%s/%s", SHARE_DIR, filename);
    
    // Check if file exists
    if (access(filepath, F_OK) != -1) {
        add_file_to_peer(filename);
        register_with_tracker("127.0.0.1", 8000, filename);
    } else {
        printf("File does not exist in the shared directory.\n");
    }
}
int main() {
    init_tracker();
    init_peer("127.0.0.1", 8001);

    // Start tracker in a separate thread
    pthread_t tracker_thread;
    int tracker_port = 8000;
    pthread_create(&tracker_thread, NULL, (void *(*)(void *))start_tracker, &tracker_port);

    // Start peer server in a separate thread
    pthread_t peer_server_thread;
    pthread_create(&peer_server_thread, NULL, start_peer_server, NULL);

    int choice;
    char filename[MAX_FILENAME];

    while(1) {
        print_menu();
        scanf("%d", &choice);

        switch(choice) {
            case 1:
                register_file();
                break;
            case 2:
                printf("Enter filename to search and download: ");
                scanf("%s", filename);
                search_and_download("127.0.0.1", 8000, filename);
                break;
            case 3:
                list_shared_files();
                break;
            case 4:
                discover_peers("127.0.0.1", 8000);
                break;
            case 5:
                printf("Exiting...\n");
                goto cleanup;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }

cleanup:
    // TODO: Implement proper shutdown of tracker and peer server threads
    pthread_cancel(tracker_thread);
    pthread_cancel(peer_server_thread);
    pthread_join(tracker_thread, NULL);
    pthread_join(peer_server_thread, NULL);

    return 0;
}

