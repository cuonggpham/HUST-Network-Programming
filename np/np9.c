/*
 * CHUONG TRINH: UDP Server - Nhan va gui lai du lieu (Echo Server)
 *
 * KIEN THUC LAP TRINH MANG:
 * - UDP server chi can bind() va recvfrom(), khong can listen() va accept()
 * - Echo server: Nhan du lieu va gui lai cho nguoi gui
 * - Stateless protocol: Khong ghi nho trang thai cua client
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
typedef struct sockaddr_IN SOCKADDR_IN;

int main() {
  // SOCK_DGRAM: Datagram socket cho UDP
  // Khac voi SOCK_STREAM (TCP) - khong co connection, gui tung packet rieng le
  int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  int d = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  SOCKADDR_IN saddr, caddr;
  caddr.sin_family = AF_INET;
  caddr.sin_port = htons(5000);
  caddr.sin_addr.s_addr = inet_addr("0.0.0.0");

  // UDP server: Chi can bind() la du de nhan du lieu
  // Khong can listen() va accept() nhu TCP server
  if (bind(d, (SOCKADDR *)&caddr, sizeof(SOCKADDR)) == 0) {
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(5000);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    while (0 == 0) {
      char buffer[1024] = {0};
      SOCKADDR_IN tmpAddr;
      int tmpLen = sizeof(SOCKADDR_IN);
      printf("Receiving...\n");

      // recvfrom() blocking: Cho doi cho den khi co du lieu
      // tmpAddr duoc dien tu dong voi thong tin cua nguoi gui
      int received = recvfrom(d, buffer, sizeof(buffer) - 1, 0,
                              (SOCKADDR *)&tmpAddr, &tmpLen);
      if (received > 0) {
        printf("Received: %d bytes from %s: %s\n", received,
               inet_ntoa(tmpAddr.sin_addr), buffer);

        // Thay doi port truoc khi gui lai
        // Client dang lang nghe tren port 6000, khong phai port gui
        tmpAddr.sin_port = htons(6000);

        // sendto(): Gui du lieu ve cho nguoi gui (su dung dia chi tu recvfrom)
        sendto(s, buffer, strlen(buffer), 0, (SOCKADDR *)&tmpAddr,
               sizeof(SOCKADDR));
      } else
        break;
    }
  } else {
    printf("Failed to bind\n");
  }

  close(d);
  close(s);
}