/*
 * CHUONG TRINH: TCP Client co ban - Ket noi va gui/nhan du lieu
 *
 * KIEN THUC LAP TRINH MANG:
 * - socket(): Tao socket TCP
 * - connect(): Ket noi toi server voi IP va port cu the
 * - send()/recv(): Gui va nhan du lieu trong vong lap
 * - inet_addr(): Chuyen chuoi IP thanh binary format
 * - htons(): Host to Network Short - Chuyen port sang network byte order
 *
 * KIEN THUC C ADVANCE:
 * - typedef: Tao ten moi cho kieu du lieu de code ngan gon hon
 * - Infinite loop: while(0==0) tuong duong while(true)
 * - memset(): Xoa du lieu trong buffer
 */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

// typedef giup dinh nghia ten ngan gon cho struct
typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

int main()
{
    // Tao socket TCP
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // Cau hinh dia chi server can ket noi
    SOCKADDR_IN saddr;
    saddr.sin_family = AF_INET;                       // IPv4
    saddr.sin_port = htons(5000);                     // Port 5000, chuyen sang network byte order
    saddr.sin_addr.s_addr = inet_addr("172.20.32.1"); // IP cua server

    // Ket noi toi server
    // (SOCKADDR*) la type casting de convert sang loai generic sockaddr
    int error = connect(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR_IN));
    if (error == 0)
    {
        while (0 == 0)
        {
            char buffer[1024] = {0};
            fgets(buffer, sizeof(buffer) - 1, stdin);
            send(s, buffer, strlen(buffer), 0);
            memset(buffer, 0, sizeof(buffer));
            printf("Waiting for data...\n");
            int r = recv(s, buffer, sizeof(buffer) - 1, 0);
            if (r >= 0)
                printf("Received %d bytes: %s\n", r, buffer);
            else
            {
                printf("Bye!\n");
                close(s);
                break;
            }
        }
    }
    else
    {
        printf("Failed to connect!\n");
    }
}