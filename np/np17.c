/*
 * CHUONG TRINH: Multi-thread Chat Server - Dung thread de xu ly client
 *
 * KIEN THUC LAP TRINH MANG:
 * - Multi-thread server: Moi client duoc xu ly boi mot thread
 * - Thread nhe hon process, dung chung bo nho
 * - Broadcast message: Gui tin nhan den tat ca client khac
 *
 * KIEN THUC C ADVANCE:
 * - Thread vs Process: Thread nhe hon, shared memory, nhanh hon
 * - Global array access: Tat ca thread deu truy cap duoc mang toan cuc
 * - Thread safety: Can chu y khi nhieu thread truy cap cung du lieu
 * - Detached thread: Thread tu giai phong khi ket thuc
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

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

// Bien toan cuc luu socket cua tat ca client
// Tat ca thread deu co the truy cap
int g_clientSockets[1024] = {0};
int g_clientCount = 0;

// Ham xu ly moi client, chay trong thread rieng
void *ClientThread(void *arg)
{
    // Lay socket cua client tu argument
    int c = *((int *)arg);

    while (0 == 0)
    {
        char buffer[1024] = {0};
        printf("Waiting for data...\n");
        int r = recv(c, buffer, sizeof(buffer) - 1, 0);

        if (r > 0)
        {
            // Broadcast message den tat ca client khac
            // Duyet qua mang toan cuc chua tat ca socket
            for (int i = 0; i < g_clientCount; i++)
            {
                // Chi gui cho client khac (khong gui lai cho nguoi gui)
                if (g_clientSockets[i] != c)
                {
                    send(g_clientSockets[i], buffer, strlen(buffer), 0);
                }
            }
        }
        else
            break; // Client ngat ket noi (recv tra ve 0 hoac -1)
    }
    printf("A client has disconnected!\n");
    // Thread tu dong ket thuc khi thoat ham
}

int main()
{
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(9999);
    saddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    if (bind(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR)) == 0)
    {
        listen(s, 10);
        while (0 == 0)
        {
            printf("Wait for a client...\n");
            int c = accept(s, NULL, 0);
            g_clientSockets[g_clientCount++] = c;
            pthread_t tid = 0;
            int *arg = (int *)calloc(1, sizeof(int));
            *arg = c;
            pthread_create(&tid, NULL, ClientThread, (void *)arg);
        }
    }
    else
    {
        printf("Failed to bind!\n");
    }
    close(s);
}