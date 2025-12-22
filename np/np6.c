/*
 * CHUONG TRINH: TCP Server co ban - Lang nghe va xu ly ket noi
 *
 * KIEN THUC LAP TRINH MANG:
 * - bind(): Gan socket voi dia chi IP va port cu the
 * - listen(): Chuyen socket sang che do lang nghe ket noi
 * - accept(): Chap nhan ket noi tu client, tao socket moi cho client
 * - 0.0.0.0: Dia chi dac biet, lang nghe tren moi interface mang
 */

#include <arpa/inet.h>
#include <malloc.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

int main() {
  int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  SOCKADDR_IN saddr, caddr; // saddr: server address, caddr: client address
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(8888);

  // 0.0.0.0 (INADDR_ANY): Lang nghe tren TAT CA cac network interface
  // Server co nhieu IP (WiFi, Ethernet, ...) se nhan ket noi tu moi IP
  saddr.sin_addr.s_addr = inet_addr("0.0.0.0");

  // bind(): Gan socket voi dia chi local (IP + port)
  // Sau khi bind, socket "so huu" port nay, process khac khong the dung
  // Tra ve 0 neu thanh cong, -1 neu loi (vd: port da duoc su dung)
  int error = bind(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR_IN));
  if (error == 0) {
    // listen(): Chuyen socket sang passive mode (cho doi ket noi)
    // Backlog = 10: Hang doi chua toi da 10 ket noi dang cho xu ly
    // Ket noi thu 11 (khi hang doi day) se bi tu choi
    listen(s, 10);

    while (0 == 0) {
      int clen = sizeof(SOCKADDR_IN);

      // accept(): Blocking call - Cho doi va chap nhan ket noi moi
      // Tra ve MOT SOCKET MOI (d) de giao tiep voi client nay
      // caddr duoc dien thong tin cua client (IP, port)
      // clen la input/output: truyen vao kich thuoc, nhan ve kich thuoc thuc
      int d = accept(s, (SOCKADDR *)&caddr, &clen);
      if (d > 0) {
        char *welcome = "Hello my first TCP server!\n";
        send(d, welcome, strlen(welcome), 0);

        char buffer[1024] = {0};
        int r = recv(d, buffer, sizeof(buffer) - 1, 0);
        if (r > 0) {
          printf("Received %d bytes: %s\n", r, buffer);
          // Dong socket client sau khi xu ly xong
          close(d);
        } else {
          printf("Failed to received!\n");
          close(d);
        }
      } else {
        printf("Failed to accept!\n");
        close(s);
        return 0;
      }
    }
  } else {
    // Loi bind thuong do: port da duoc su dung, khong co quyen (port < 1024)
    printf("Failed to bind!\n");
    close(s);
    return 0;
  }
}