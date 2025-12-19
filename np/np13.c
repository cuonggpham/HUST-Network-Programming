/*
 * CHUONG TRINH: Multi-process TCP Server - Dung fork de xu ly nhieu client
 *
 * KIEN THUC LAP TRINH MANG:
 * - Concurrent server: Xu ly nhieu client dong thoi
 * - Fork model: Moi client duoc xu ly boi mot process rieng
 * - close() socket trong child va parent dung cach
 *
 * KIEN THUC C ADVANCE:
 * - fork() trong server: Parent tiep tuc accept(), child xu ly client
 * - File descriptor inheritance: Child ke thua tat ca FD cua parent
 * - Closing sockets: Parent dong client socket, child dong server socket
 * - Zombie process: Child process sau khi exit can duoc parent thu hoi
 */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

int main()
{
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8888);
    saddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    if (bind(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR)) == 0)
    {
        listen(s, 10);
        while (0 == 0)
        {
            SOCKADDR_IN caddr;
            int clen = sizeof(SOCKADDR_IN);
            printf("Parent: waiting for connection!\n");
            int d = accept(s, (SOCKADDR *)&caddr, &clen);
            printf("Parent: one more client connected!\n");

            // Tao process con de xu ly client nay
            if (fork() == 0)
            {
                // CHILD PROCESS - Xu ly client

                // Dong server socket vi child khong can accept() nua
                // Child chi can socket d de giao tiep voi client
                close(s);

                while (0 == 0)
                {
                    char buffer[1024] = {0};
                    int received = recv(d, buffer, sizeof(buffer) - 1, 0);
                    if (received > 0)
                    {
                        printf("Child: received %d bytes: %s\n", received, buffer);
                    }
                    else
                    {
                        printf("Child: disconnected!\n");
                        // exit() ket thuc child process khi client ngat ket noi
                        exit(0);
                    }
                }
            }

            // PARENT PROCESS - Tiep tuc accept() client moi
            // Dong client socket vi parent khong xu ly client
            // Chi child moi xu ly client
            close(d);
        }
    }
    else
    {
        printf("Failed to bind!\n");
        close(s);
    }
}