#include <stdio.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <ctype.h>

#define MAX 1024
#define PORT 16001
#define SA struct sockaddr 

int is_palindrome(const char *str) {
    int i = 0, j = strlen(str) - 1;
    while (i < j) {
        if (str[i] != str[j]) return 0;
        i++;
        j--;
    }
    return 1;
}

void handle_numbers(int sockfd, char *buff) {
    int even_count = 0, odd_count = 0;
    char *token = strtok(buff, " ");
    printf("%c",token[0]);
    while (token != NULL && token[0]!= ' ') {
        int num = atoi(token);
        if (num % 2 == 0) even_count++;
        else odd_count++;
        token = strtok(NULL, " ");
    }
    sprintf(buff, "Even numbers: %d, Odd numbers: %d\n", even_count, odd_count);
    write(sockfd, buff, strlen(buff));
}

void handle_palindromes(int sockfd, char *buff) {
    int palindrome_count = 0;
    char *token = strtok(buff, " \n");
    while (token != NULL) {
        if (is_palindrome(token)) palindrome_count++;
        token = strtok(NULL, " \n");
    }
    sprintf(buff, "Palindrome count: %d\n", palindrome_count);
    write(sockfd, buff, strlen(buff));
}

void handle_longest_string(int sockfd, char *buff) {
    char longest[MAX] = "";
    char *token = strtok(buff, " \n");
    while (token != NULL) {
        if (strlen(token) > strlen(longest)) {
            strcpy(longest, token);
        }
        token = strtok(NULL, " \n");
    }
    sprintf(buff, "Longest string: %s\n", longest);
    write(sockfd, buff, strlen(buff));
}

void handle_request(int sockfd) {
    char buff[MAX];
    int n;
    
    for (;;) {
        bzero(buff, MAX);
        n = read(sockfd, buff, sizeof(buff));
        if (n <= 0) break;
        
        if (strncmp("exit", buff, 4) == 0) {
            printf("\033[0;31m");
            printf("Client is exiting...\n");
            break;
        }
        
        printf("Received: %s", buff);
        
      if (isdigit(buff[0])) {
    handle_numbers(sockfd, buff);
} else {
    char request_type = buff[0];
    char* data = buff + 2; 

    switch(request_type) {
        case 'P':  
            handle_palindromes(sockfd, data);
            break;
        case 'L': 
            handle_longest_string(sockfd, data);
            break;
        default:
            sprintf(buff, "Invalid request type\n");
            write(sockfd, buff, strlen(buff));
    }
}
    }
}

int main() {
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    bzero(&servaddr, sizeof(servaddr));
    
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }
    
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    printf("Server listening on port %d...\n", PORT);
    
    socklen_t len = sizeof(cli);
    
    for (;;) {
        connfd = accept(sockfd, (SA*)&cli, &len);
        if (connfd < 0) {
            printf("server accept failed...\n");
            exit(0);
        }
        printf("Server accepted a client...\n");
        
        handle_request(connfd);
        close(connfd);
    }
    
    close(sockfd);
    return 0;
}