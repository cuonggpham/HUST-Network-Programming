/*
 * CHUONG TRINH: TCP Server thuc thi lenh - Nhan lenh tu client va thuc thi
 *
 * KIEN THUC LAP TRINH MANG:
 * - Xu ly nhieu request tu cung mot client trong vong lap
 * - Gui ket qua thuc thi lenh qua socket
 * - Remote command execution (RCE) - CANH BAO: Loi bao mat nghiem trong!
 */

#include <arpa/inet.h>
#include <malloc.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

int main() {
  int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  SOCKADDR_IN saddr, caddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(9999);
  saddr.sin_addr.s_addr = inet_addr("0.0.0.0");

  int error = bind(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR_IN));
  if (error == 0) {
    listen(s, 10);
    while (0 == 0) {
      int clen = sizeof(SOCKADDR_IN);
      printf("Waiting for client!\n");
      int d = accept(s, (SOCKADDR *)&caddr, &clen);
      if (d > 0) {
        // Vong lap xu ly nhieu request tu cung 1 client
        // Keep-alive connection: Giu ket noi de xu ly nhieu lenh
        while (0 == 0) {
          printf("Waiting for command...\n");
          char buffer[1024] = {0};
          int r = recv(d, buffer, sizeof(buffer) - 1, 0);
          if (r > 0) {
            printf("Received %d bytes: %s\n", r, buffer);

            // Loai bo CRLF o cuoi lenh (network protocols thuong dung \r\n)
            // strlen(buffer) - 1: Vi tri ky tu cuoi cung truoc null terminator
            while (buffer[strlen(buffer) - 1] == '\r' ||
                   buffer[strlen(buffer) - 1] == '\n') {
              buffer[strlen(buffer) - 1] = 0;
            }

            // Pointer arithmetic: buffer + strlen(buffer) = vi tri cuoi chuoi
            // sprintf() ghi tiep vao cuoi, tao chuoi: "lenh > out.txt"
            sprintf(buffer + strlen(buffer), "%s", " > out.txt");

            // system(): Thuc thi lenh shell
            // CANH BAO NGHIEM TRONG: Command Injection vulnerability!
            // Attacker co the gui: "rm -rf / ;" de pha huy he thong
            // KHONG BAO GIO dung input tu user truc tiep voi system()
            system(buffer);

            FILE *f = fopen("out.txt", "rt");

            // Doc tung dong tu file ket qua va gui ve client
            while (!feof(f)) {
              memset(buffer, 0, sizeof(buffer));
              fgets(buffer, sizeof(buffer) - 1, f);
              send(d, buffer, strlen(buffer), 0);
            }
          } else {
            printf("Failed to received!\n");
            close(d);
            break;
          }
        }
      } else {
        printf("Failed to accept!\n");
        close(s);
        return 0;
      }
    }
  } else {
    printf("Failed to bind!\n");
    close(s);
    return 0;
  }
}