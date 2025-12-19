/*
 * CHUONG TRINH: I/O Multiplexing voi select() - Xu ly nhieu socket cung luc
 *
 * KIEN THUC LAP TRINH MANG:
 * - select(): Cho doi su kien tren nhieu socket dong thoi
 * - I/O multiplexing: Mot thread xu ly nhieu ket noi
 * - Non-blocking I/O: Khong can tao thread/process cho moi client
 * - FD_SET: Cac macro de thao tac voi file descriptor set
 *
 * KIEN THUC C ADVANCE:
 * - fd_set: Tap hop cac file descriptor de theo doi
 * - FD_ZERO(): Xoa tat ca FD khoi set
 * - FD_SET(): Them FD vao set
 * - FD_ISSET(): Kiem tra FD co trong set khong
 * - FD_SETSIZE: So luong FD toi da (thuong la 1024)
 * - select() block cho den khi co su kien tren bat ky FD nao
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
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

int g_clientSockets[1024] = {0}; // Luu tat ca client socket
int g_clientCount = 0;

int main()
{
    SOCKADDR_IN saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8888);
    saddr.sin_addr.s_addr = 0;
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (bind(s, (SOCKADDR *)&saddr, sizeof(saddr)) == 0)
    {
        listen(s, 10);
        fd_set read; // Tap hop file descriptor can theo doi

        while (0 == 0)
        {
            // FD_ZERO() xoa tat ca FD khoi set (bat dau tu dau)
            FD_ZERO(&read);

            // FD_SET() them server socket vao set de theo doi ket noi moi
            FD_SET(s, &read);

            // Them tat ca client socket vao set de theo doi du lieu
            for (int i = 0; i < g_clientCount; i++)
            {
                FD_SET(g_clientSockets[i], &read);
            }

            // select() BLOCK cho den khi co su kien tren bat ky FD nao
            // Tham so 1: So FD toi da + 1 (dung FD_SETSIZE)
            // Tham so 2: Set FD doc, 3: ghi, 4: exception, 5: timeout
            // Tra ve so FD co su kien, -1 neu loi
            int n = select(FD_SETSIZE, &read, NULL, NULL, NULL);

            if (n > 0) // Co it nhat 1 FD san sang
            {
                // FD_ISSET() kiem tra xem FD co trong set khong
                // Neu server socket ready -> co client moi ket noi
                if (FD_ISSET(s, &read))
                {
                    int c = accept(s, NULL, NULL);
                    printf("A new client connected.\n");
                    g_clientSockets[g_clientCount++] = c;
                }

                // Kiem tra xem client socket nao co du lieu
                for (int i = 0; i < g_clientCount; i++)
                {
                    if (FD_ISSET(g_clientSockets[i], &read))
                    {
                        // Client nay gui du lieu
                        char data[1024] = {0};
                        recv(g_clientSockets[i], data, sizeof(data) - 1, 0);

                        // Broadcast den cac client khac
                        for (int j = 0; j < g_clientCount; j++)
                        {
                            if (j != i)
                                send(g_clientSockets[j], data, strlen(data), 0);
                        }
                    }
                }
            }
        }
    }
    else
        printf("Failed to bind.\n");

    close(s);
}