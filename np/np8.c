/*
 * CHUONG TRINH: UDP Client - Gui va nhan du lieu khong ket noi
 *
 * KIEN THUC LAP TRINH MANG:
 * - UDP (User Datagram Protocol): Giao thuc khong ket noi, khong dam bao
 * - SOCK_DGRAM: Loai socket cho UDP
 * - sendto(): Gui du lieu den dia chi cu the (khong can connect truoc)
 * - recvfrom(): Nhan du lieu va biet nguon gui
 * - UDP can 2 socket: 1 de gui, 1 de nhan (bind port rieng)
 *
 * KIEN THUC C ADVANCE:
 * - Khac biet giua TCP (stream) va UDP (datagram)
 * - inet_ntoa(): Chuyen dia chi IP thanh chuoi de hien thi
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
    caddr.sin_port = htons(6000);
    caddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    if (bind(d, (SOCKADDR *)&caddr, sizeof(SOCKADDR)) == 0)
    {
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(5000);
        saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        while (0 == 0)
        {
            printf("Type something: ");
            char buffer[1024] = {0};
            fgets(buffer, sizeof(buffer) - 1, stdin);

            // sendto() gui du lieu qua UDP den dia chi chi dinh
            // Khong can connect() truoc, chi can biet dia chi dich
            // Tham so 5: Dia chi dich, Tham so 6: Kich thuoc struct dia chi
            int sent = sendto(s, buffer, strlen(buffer), 0, (SOCKADDR *)&saddr, sizeof(SOCKADDR));
            printf("Sent: %d bytes\n", sent);

            memset(buffer, 0, sizeof(buffer));
            SOCKADDR_IN tmpAddr;
            int tmpLen = sizeof(SOCKADDR_IN);
            printf("Receiving...");

            // recvfrom() nhan du lieu va luu thong tin nguon gui vao tmpAddr
            // tmpAddr se chua IP va port cua nguoi gui
            // tmpLen la input/output parameter (truyen vao kich thuoc, tra ve kich thuoc thuc te)
            int received = recvfrom(d, buffer, sizeof(buffer) - 1, 0, (SOCKADDR *)&tmpAddr, &tmpLen);
            if (received > 0)
                printf("Received: %d bytes from %s: %s\n", received, inet_ntoa(tmpAddr.sin_addr), buffer);
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