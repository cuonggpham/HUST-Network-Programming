/*
 * CHUONG TRINH: TCP Server co ban - Lang nghe va xu ly ket noi
 *
 * KIEN THUC LAP TRINH MANG:
 * - bind(): Gan socket voi dia chi IP va port cu the
 * - listen(): Chuyen socket sang che do lang nghe ket noi
 * - accept(): Chap nhan ket noi tu client, tao socket moi cho client
 * - 0.0.0.0: Dia chi dac biet, lang nghe tren moi interface mang
 *
 * KIEN THUC C ADVANCE:
 * - Error handling: Kiem tra gia tri tra ve cua cac ham socket
 * - Resource cleanup: Dong socket khi khong dung nua
 */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

int main()
{
    // Tao socket TCP cho server
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    SOCKADDR_IN saddr, caddr; // saddr: server address, caddr: client address
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8888);                 // Server lang nghe tren port 8888
    saddr.sin_addr.s_addr = inet_addr("0.0.0.0"); // 0.0.0.0 = lang nghe tren moi IP

    // bind() gan socket voi dia chi va port
    // Sau khi bind, socket se lang nghe tren port nay
    int error = bind(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR_IN));
    if (error == 0)
    {
        // listen() chuyen socket sang che do lang nghe
        // Tham so 2 (backlog=10): So luong ket noi toi da cho trong hang doi
        listen(s, 10);

        while (0 == 0) // Vong lap vo han de phuc vu nhieu client
        {
            int clen = sizeof(SOCKADDR_IN);

            // accept() cho doi va chap nhan ket noi tu client
            // Ham nay BLOCK cho den khi co client ket noi
            // Tra ve socket moi (d) de giao tiep voi client nay
            // caddr se chua thong tin dia chi cua client
            int d = accept(s, (SOCKADDR *)&caddr, &clen);
            if (d > 0)
            {
                char *welcome = "Hello my first TCP server!\n";
                send(d, welcome, strlen(welcome), 0);
                char buffer[1024] = {0};
                int r = recv(d, buffer, sizeof(buffer) - 1, 0);
                if (r > 0)
                {
                    printf("Received %d bytes: %s\n", r, buffer);
                    close(d);
                }
                else
                {
                    printf("Failed to received!\n");
                    close(d);
                }
            }
            else
            {
                printf("Failed to accept!\n");
                close(s);
                return 0;
            }
        }
    }
    else
    {
        printf("Failed to bind!\n");
        close(s);
        return 0;
    }
}