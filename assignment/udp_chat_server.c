#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_PORT 5000
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 128

int main(void)
{
    int sockfd;
    struct sockaddr_in server_addr;
    struct sockaddr_in clients[MAX_CLIENTS];
    int client_count = 0;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(struct sockaddr_in);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("UDP chat server listening on port %d\n", SERVER_PORT);

    while (1)
    {
        struct sockaddr_in cli_addr;
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&cli_addr, &addr_len);
        if (n < 0)
        {
            perror("recvfrom");
            continue;
        }
        buffer[n] = '\0';

        int found = 0;
        for (int i = 0; i < client_count; ++i)
        {
            if (clients[i].sin_addr.s_addr == cli_addr.sin_addr.s_addr &&
                clients[i].sin_port == cli_addr.sin_port)
            {
                found = 1;
                break;
            }
        }
        if (!found)
        {
            if (client_count < MAX_CLIENTS)
            {
                clients[client_count++] = cli_addr;
                char ipstr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &cli_addr.sin_addr, ipstr, sizeof(ipstr));
                printf("New client %s:%d registered (total %d)\n", ipstr, ntohs(cli_addr.sin_port), client_count);
            }
            else
            {
                fprintf(stderr, "Max clients reached, ignoring new client\n");
            }
        }

        for (int i = 0; i < client_count; ++i)
        {
            if (clients[i].sin_addr.s_addr == cli_addr.sin_addr.s_addr && clients[i].sin_port == cli_addr.sin_port)
            {
                continue;
            }
            if (sendto(sockfd, buffer, n, 0, (struct sockaddr *)&clients[i], sizeof(struct sockaddr_in)) < 0)
            {
                perror("sendto");
            }
        }

        char ipbuf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cli_addr.sin_addr, ipbuf, sizeof(ipbuf));
        printf("%s:%d -> %s\n", ipbuf, ntohs(cli_addr.sin_port), buffer);
    }

    close(sockfd);
    return 0;
}
