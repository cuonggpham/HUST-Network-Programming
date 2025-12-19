/*
 * CHUONG TRINH: TCP Server thuc thi lenh - Nhan lenh tu client va thuc thi
 *
 * KIEN THUC LAP TRINH MANG:
 * - Xu ly nhieu request tu cung mot client trong vong lap
 * - Gui ket qua thuc thi lenh qua socket
 *
 * KIEN THUC C ADVANCE:
 * - system(): Thuc thi lenh shell va luu ket qua ra file
 * - sprintf(): Them text vao chuoi co san
 * - File I/O: Doc file va gui noi dung qua mang
 * - Security risk: Code nay co loi bao mat (command injection)
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
    SOCKADDR_IN saddr, caddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(9999);
    saddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    int error = bind(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR_IN));
    if (error == 0)
    {
        listen(s, 10);
        while (0 == 0)
        {
            int clen = sizeof(SOCKADDR_IN);
            printf("Waiting for client!\n");
            int d = accept(s, (SOCKADDR *)&caddr, &clen);
            if (d > 0)
            {
                while (0 == 0)
                {
                    printf("Waiting for command...\n");
                    char buffer[1024] = {0};
                    int r = recv(d, buffer, sizeof(buffer) - 1, 0);
                    if (r > 0)
                    {
                        printf("Received %d bytes: %s\n", r, buffer);

                        // Loai bo ki tu xuong dong o cuoi lenh
                        while (buffer[strlen(buffer) - 1] == '\r' || buffer[strlen(buffer) - 1] == '\n')
                        {
                            buffer[strlen(buffer) - 1] = 0;
                        }

                        // Them redirect output vao file
                        // sprintf(buffer + strlen(buffer), ...) de them vao cuoi chuoi
                        sprintf(buffer + strlen(buffer), "%s", " > out.txt");

                        // Thuc thi lenh shell - CANH BAO: Loi bao mat!
                        // Khong nen dung truc tiep input tu user cho system()
                        system(buffer);
                        FILE *f = fopen("out.txt", "rt");
                        while (!feof(f))
                        {
                            memset(buffer, 0, sizeof(buffer));
                            fgets(buffer, sizeof(buffer) - 1, f);
                            send(d, buffer, strlen(buffer), 0);
                        }
                    }
                    else
                    {
                        printf("Failed to received!\n");
                        close(d);
                        break;
                    }
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