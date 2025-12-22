/*
 * CHUONG TRINH: UDP Chat Server - Phat tan tin nhan den tat ca client
 *
 * KIEN THUC LAP TRINH MANG:
 * - Quan ly danh sach client bang mang toan cuc
 * - Broadcast message: Gui tin nhan den tat ca client tru nguoi gui
 * - Client discovery: Tu dong them client moi khi nhan tin nhan dau tien
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

// Global array: Mang toan cuc luu dia chi cua tat ca client
// Tat ca ham trong file deu co the truy cap
// = {0}: Khoi tao tat ca phan tu = 0
SOCKADDR_IN g_Addr[1024] = {0};

// Global variable: Dem so luong client hien co
int g_count = 0;

int main() {
  SOCKADDR_IN saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(5000);
  saddr.sin_addr.s_addr = inet_addr("0.0.0.0");

  int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (bind(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR)) == 0) {
    while (0 == 0) {
      char buffer[1024] = {0};
      SOCKADDR_IN tmpAddr;
      int tmpLen = sizeof(SOCKADDR_IN);
      printf("Receiving...\n");
      recvfrom(s, buffer, sizeof(buffer) - 1, 0, (SOCKADDR *)&tmpAddr, &tmpLen);
      printf("Received: %s from %s\n", buffer, inet_ntoa(tmpAddr.sin_addr));

      // So sanh dia chi IP de kiem tra client da ton tai chua
      // sin_addr.s_addr la so 32-bit (unsigned long) dai dien cho IP
      // So sanh truc tiep nhu so nguyen, nhanh hon strcmp() chuoi IP
      int found = 0;
      for (int i = 0; i < g_count; i++) {
        // So sanh 2 IP address bang phep == tren 32-bit integer
        if (g_Addr[i].sin_addr.s_addr == tmpAddr.sin_addr.s_addr) {
          found = 1;
          break;
        }
      }

      // Them client moi vao danh sach neu chua co
      if (found == 0) {
        // memcpy(): Copy toan bo struct (tat ca fields)
        // Khac voi gan (=) o cho co the copy struct co padding
        memcpy(&g_Addr[g_count], &tmpAddr, sizeof(tmpAddr));
        g_count++;
        printf("New client added!\n");
      }

      // Broadcast: Gui tin nhan den TAT CA client tru nguoi gui
      for (int i = 0; i < g_count; i++) {
        // Chi gui cho client KHAC nguoi gui (tranh echo lai)
        if (g_Addr[i].sin_addr.s_addr != tmpAddr.sin_addr.s_addr) {
          sendto(s, buffer, strlen(buffer), 0, (SOCKADDR *)&g_Addr[i],
                 sizeof(SOCKADDR));
        }
      }
    }
  } else
    printf("Faild to bind!\n");

  close(s);
}