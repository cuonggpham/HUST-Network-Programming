/*
 * CHUONG TRINH: UDP Chat Server - Phat tan tin nhan den tat ca client
 *
 * KIEN THUC LAP TRINH MANG:
 * - Quan ly danh sach client bang mang toan cuc
 * - Broadcast message: Gui tin nhan den tat ca client tru nguoi gui
 * - Kiem tra client moi bang so sanh dia chi IP
 *
 * KIEN THUC C ADVANCE:
 * - Global array: Mang toan cuc de luu thong tin client
 * - memcpy(): Sao chep struct address
 * - So sanh IP: sin_addr.s_addr la so 32-bit dai dien cho IP
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

// Mang toan cuc luu dia chi cua tat ca client da ket noi
SOCKADDR_IN g_Addr[1024] = {0};
int g_count = 0; // So luong client hien tai

int main()
{
    SOCKADDR_IN saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(5000);
    saddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (bind(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR)) == 0)
    {
        while (0 == 0)
        {
            char buffer[1024] = {0};
            SOCKADDR_IN tmpAddr;
            int tmpLen = sizeof(SOCKADDR_IN);
            printf("Receiving...\n");
            recvfrom(s, buffer, sizeof(buffer) - 1, 0, (SOCKADDR *)&tmpAddr, &tmpLen);
            printf("Received: %s from %s\n", buffer, inet_ntoa(tmpAddr.sin_addr));

            // Kiem tra xem client da ton tai trong danh sach chua
            // So sanh bang sin_addr.s_addr (32-bit integer dai dien IP)
            int found = 0;
            for (int i = 0; i < g_count; i++)
            {
                if (g_Addr[i].sin_addr.s_addr == tmpAddr.sin_addr.s_addr)
                {
                    found = 1;
                    break;
                }
            }

            // Neu la client moi thi them vao danh sach
            if (found == 0)
            {
                memcpy(&g_Addr[g_count], &tmpAddr, sizeof(tmpAddr));
                g_count++;
                printf("New client added!\n");
            }

            // Gui tin nhan den tat ca client tru nguoi gui (broadcast)
            for (int i = 0; i < g_count; i++)
            {
                // Chi gui cho nhung client khac nguoi gui
                if (g_Addr[i].sin_addr.s_addr != tmpAddr.sin_addr.s_addr)
                {
                    sendto(s, buffer, strlen(buffer), 0, (SOCKADDR *)&g_Addr[i], sizeof(SOCKADDR));
                }
            }
        }
    }
    else
        printf("Faild to bind!\n");

    close(s);
}