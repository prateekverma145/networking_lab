#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX 1024
#define PORT 12345
#define SA struct sockaddr

void HandleRequest(int sockfd)
{
    char buff[MAX];
    int arr[100];
    int n, count_even, count_odd, count_palindrome;
    char longest_string[MAX];

    for (;;)
    {
        bzero(buff, MAX);
        read(sockfd, buff, sizeof(buff));

        if (strncmp("exit", buff, 4) == 0)
        {
            printf("Client is Exiting...\n");
            close(sockfd);
            break;
        }
        else if (strncmp("NUMBERS", buff, 7) == 0)
        {
            count_even = count_odd = 0;
            char *token = strtok(buff + 8, " ");
            while (token != NULL)
            {
                int num = atoi(token);
                if (num % 2 == 0)
                    count_even++;
                else
                    count_odd++;
                token = strtok(NULL, " ");
            }
            sprintf(buff, "EVEN: %d, ODD: %d", count_even, count_odd);
            write(sockfd, buff, sizeof(buff));
        }
        else if (strncmp("STRINGS_PALINDROME", buff, 18) == 0)
        {
            count_palindrome = 0;
            char *token = strtok(buff + 19, " ");
            while (token != NULL)
            {
                int len = strlen(token);
                int palindrome = 1;
                for (int i = 0; i < len / 2; i++)
                {
                    if (token[i] != token[len - i - 1])
                    {
                        palindrome = 0;
                        break;
                    }
                }
                if (palindrome)
                    count_palindrome++;
                token = strtok(NULL, " ");
            }
            sprintf(buff, "PALINDROMES: %d", count_palindrome);
            write(sockfd, buff, sizeof(buff));
        }
        else if (strncmp("STRINGS_LONGEST", buff, 15) == 0)
        {
            bzero(longest_string, sizeof(longest_string));
            char *token = strtok(buff + 16, " ");
            while (token != NULL)
            {
                if (strlen(token) > strlen(longest_string))
                {
                    strcpy(longest_string, token);
                }
                token = strtok(NULL, " ");
            }
            sprintf(buff, "LONGEST: %s", longest_string);
            write(sockfd, buff, sizeof(buff));
        }
    }
}

int main()
{
    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("Socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0)
    {
        printf("Socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");

    if ((listen(sockfd, 5)) != 0)
    {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");
    len = sizeof(cli);

    for (;;)
    {
        connfd = accept(sockfd, (SA *)&cli, &len);
        if (connfd < 0)
        {
            printf("Server accept failed...\n");
            exit(0);
        }
        else
            printf("Server accepted the client...\n");

        HandleRequest(connfd);
    }

    close(sockfd);
}
