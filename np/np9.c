/*
 * CHUONG TRINH: UDP Server - Nhan va gui lai du lieu
 *
 * KIEN THUC LAP TRINH MANG:
 * - UDP server chi can bind() va recvfrom(), khong can listen() va accept()
 * - Echo server: Nhan du lieu va gui lai cho nguoi gui
 * - Thay doi port truoc khi gui lai (port 6000)
 *
 * KIEN THUC C ADVANCE:
 * - UDP don gian hon TCP vi khong can quan ly ket noi
 * - Stateless protocol: Khong ghi nho trang thai cua client
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
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int d = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SOCKADDR_IN saddr, caddr;
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(5000);
    caddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    if (bind(d, (SOCKADDR *)&caddr, sizeof(SOCKADDR)) == 0)
    {
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(5000);
        saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        while (0 == 0)
        {
            char buffer[1024] = {0};
            SOCKADDR_IN tmpAddr;
            int tmpLen = sizeof(SOCKADDR_IN);
            printf("Receiving...\n");
            int received = recvfrom(d, buffer, sizeof(buffer) - 1, 0, (SOCKADDR *)&tmpAddr, &tmpLen);
            if (received > 0)
            {
                printf("Received: %d bytes from %s: %s\n", received, inet_ntoa(tmpAddr.sin_addr), buffer);
                tmpAddr.sin_port = htons(6000);
                sendto(s, buffer, strlen(buffer), 0, (SOCKADDR *)&tmpAddr, sizeof(SOCKADDR));
            }
            else
                break;
        }
    }
    else
    {
        printf("Failed to bind\n");
    }

    close(d);
    close(s);
}