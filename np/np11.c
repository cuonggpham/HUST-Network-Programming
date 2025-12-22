/*
 * CHUONG TRINH: UDP Broadcast - Gui tin den tat ca may trong mang
 *
 * KIEN THUC LAP TRINH MANG:
 * - Broadcast: Gui du lieu den tat ca may trong cung subnet
 * - 255.255.255.255: Dia chi broadcast (limited broadcast)
 * - SO_BROADCAST: Socket option phai bat truoc khi broadcast
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
  // Bien dung de bat socket option
  // setsockopt() can con tro toi gia tri
  int on = 1;

  int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  // setsockopt(): Thiet lap tuy chon cho socket
  // Tham so 1: socket file descriptor
  // Tham so 2: level - SOL_SOCKET (socket level, khong phai IP hay TCP)
  // Tham so 3: option name - SO_BROADCAST (cho phep gui broadcast)
  // Tham so 4: con tro toi gia tri option
  // Tham so 5: kich thuoc gia tri
  // MAC DINH broadcast bi VO HIEU HOA de tranh lam tran mang
  setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof(int));

  SOCKADDR_IN saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(5000);

  // 255.255.255.255: Limited broadcast address
  // Gui den TAT CA host trong mang local
  // Router se KHONG forward packet nay ra ngoai (limited)
  // Directed broadcast (vd: 192.168.1.255) gui den 1 subnet cu the
  saddr.sin_addr.s_addr = inet_addr("255.255.255.255");

  while (0 == 0) {
    char buffer[1024] = {0};
    fgets(buffer, sizeof(buffer) - 1, stdin);

    // sendto() voi dia chi broadcast gui den moi host
    // Moi may trong LAN co port 5000 dang lang nghe deu nhan duoc
    int sent = sendto(s, buffer, strlen(buffer), 0, (SOCKADDR *)&saddr,
                      sizeof(SOCKADDR));
    printf("Sent: %d bytes\n", sent);
  }
}