/*
 * CHUONG TRINH: UDP Broadcast - Gui tin den tat ca may trong mang
 *
 * KIEN THUC LAP TRINH MANG:
 * - Broadcast: Gui du lieu den tat ca may trong cung subnet
 * - 255.255.255.255: Dia chi broadcast (limited broadcast)
 * - SO_BROADCAST: Socket option phai bat truoc khi broadcast
 * - setsockopt(): Thiet lap cac tuy chon cho socket
 *
 * KIEN THUC C ADVANCE:
 * - Socket options: Cac thiet lap dac biet cho socket
 * - Broadcast vs Unicast vs Multicast
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
    int on = 1; // Gia tri de bat option
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // setsockopt() thiet lap tuy chon cho socket
    // SOL_SOCKET: Thiet lap o muc socket (khong phai IP hay TCP)
    // SO_BROADCAST: Cho phep gui broadcast
    // Mac dinh broadcast bi vo hieu hoa de tranh lam tran mang
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof(int));

    SOCKADDR_IN saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(5000);
    // 255.255.255.255 la dia chi broadcast dac biet
    // Gui den tat ca may trong mang local
    saddr.sin_addr.s_addr = inet_addr("255.255.255.255");
    while (0 == 0)
    {
        char buffer[1024] = {0};
        fgets(buffer, sizeof(buffer) - 1, stdin);
        int sent = sendto(s, buffer, strlen(buffer), 0, (SOCKADDR *)&saddr, sizeof(SOCKADDR));
        printf("Sent: %d bytes\n", sent);
    }
}