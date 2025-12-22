/*
 * CHUONG TRINH: TCP Client co ban - Ket noi va gui/nhan du lieu
 *
 * KIEN THUC LAP TRINH MANG:
 * - socket(): Tao socket TCP
 * - connect(): Ket noi toi server voi IP va port cu the
 * - send()/recv(): Gui va nhan du lieu trong vong lap
 * - inet_addr(): Chuyen chuoi IP thanh binary format
 * - htons(): Host to Network Short - Chuyen port sang network byte order
 */

#include <arpa/inet.h>
#include <malloc.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// typedef: Tao alias (ten moi) cho kieu du lieu co san
// Giup code ngan gon hon, de doc hon
typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

int main() {
  // socket(): Tao endpoint cho giao tiep mang
  // Tra ve file descriptor (int) dai dien cho socket
  int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  // Khoi tao struct bang cach gan gia tri cho tung field
  SOCKADDR_IN saddr;
  saddr.sin_family = AF_INET; // IPv4

  // htons(): Host to Network Short (16-bit)
  // Chuyen port number tu host byte order sang network byte order (Big Endian)
  saddr.sin_port = htons(5000);

  // inet_addr(): Chuyen chuoi IP "x.x.x.x" thanh 32-bit binary (network byte
  // order) Tra ve INADDR_NONE (-1) neu chuoi IP khong hop le
  saddr.sin_addr.s_addr = inet_addr("172.20.32.1");

  // Type casting: (SOCKADDR*) chuyen con tro SOCKADDR_IN* sang SOCKADDR*
  // Can thiet vi connect() nhan generic SOCKADDR*, nhung ta dung SOCKADDR_IN
  int error = connect(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR_IN));
  if (error == 0) {
    // Infinite loop: while(0 == 0) tuong duong while(1) hoac while(true)
    while (0 == 0) {
      char buffer[1024] = {0};
      fgets(buffer, sizeof(buffer) - 1, stdin);
      send(s, buffer, strlen(buffer), 0);

      // memset(): Dat tat ca bytes trong buffer = 0
      // memset(buffer, value, size): Set 'size' bytes thanh 'value'
      memset(buffer, 0, sizeof(buffer));
      printf("Waiting for data...\n");

      int r = recv(s, buffer, sizeof(buffer) - 1, 0);
      if (r >= 0)
        printf("Received %d bytes: %s\n", r, buffer);
      else {
        printf("Bye!\n");
        close(s);
        break;
      }
    }
  } else {
    printf("Failed to connect!\n");
  }
}