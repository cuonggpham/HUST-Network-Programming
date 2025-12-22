/*
 * CHUONG TRINH: UDP Client - Gui va nhan du lieu khong ket noi
 *
 * KIEN THUC LAP TRINH MANG:
 * - UDP (User Datagram Protocol): Giao thuc khong ket noi, khong dam bao
 * - SOCK_DGRAM: Loai socket cho UDP (datagram)
 * - sendto(): Gui du lieu den dia chi cu the (khong can connect truoc)
 * - recvfrom(): Nhan du lieu va biet nguon gui
 * - UDP can bind port de nhan du lieu (khac voi TCP client)
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
  // UDP can 2 socket: s de gui, d de nhan
  // Hoac dung 1 socket nhung phai bind truoc khi gui/nhan
  int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  int d = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  SOCKADDR_IN saddr, caddr;
  caddr.sin_family = AF_INET;
  caddr.sin_port = htons(6000); // Port de nhan du lieu
  caddr.sin_addr.s_addr = inet_addr("0.0.0.0");

  // UDP client cung phai bind() neu muon nhan du lieu
  // Khac voi TCP client (connect() tu dong gan port ephemeral)
  if (bind(d, (SOCKADDR *)&caddr, sizeof(SOCKADDR)) == 0) {
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(5000); // Port cua server
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    while (0 == 0) {
      printf("Type something: ");
      char buffer[1024] = {0};
      fgets(buffer, sizeof(buffer) - 1, stdin);

      // sendto(): Gui UDP datagram den dia chi cu the
      // Khong can connect() truoc (connectionless)
      // Tham so 5: Dia chi dich (struct sockaddr*)
      // Tham so 6: Kich thuoc struct dia chi
      int sent = sendto(s, buffer, strlen(buffer), 0, (SOCKADDR *)&saddr,
                        sizeof(SOCKADDR));
      printf("Sent: %d bytes\n", sent);

      memset(buffer, 0, sizeof(buffer));
      SOCKADDR_IN tmpAddr;

      // sizeof() tra ve kich thuoc cua kieu du lieu hoac bien
      // Dung de truyen dung size cho cac ham system
      int tmpLen = sizeof(SOCKADDR_IN);
      printf("Receiving...");

      // recvfrom(): Nhan UDP datagram va lay thong tin nguon gui
      // tmpAddr (output): Chua IP va port cua nguoi gui sau khi nhan
      // tmpLen (input/output): Truyen vao kich thuoc buffer, tra ve kich thuoc
      // thuc Khac recv(): recvfrom cho biet AI gui, recv chi nhan data
      int received = recvfrom(d, buffer, sizeof(buffer) - 1, 0,
                              (SOCKADDR *)&tmpAddr, &tmpLen);
      if (received > 0)
        // inet_ntoa(): Chuyen struct in_addr thanh chuoi IP de in
        printf("Received: %d bytes from %s: %s\n", received,
               inet_ntoa(tmpAddr.sin_addr), buffer);
      else
        break;
    }
  } else {
    printf("Failed to bind\n");
  }

  // Dong ca 2 socket khi ket thuc
  close(d);
  close(s);
}