/*
 * CHUONG TRINH: Multi-process TCP Server - Dung fork de xu ly nhieu client
 *
 * KIEN THUC LAP TRINH MANG:
 * - Concurrent server: Xu ly nhieu client dong thoi
 * - Fork model: Moi client duoc xu ly boi mot process rieng
 * - Connection handling: Parent tiep tuc accept, child xu ly client
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
  SOCKADDR_IN saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(8888);
  saddr.sin_addr.s_addr = inet_addr("0.0.0.0");

  if (bind(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR)) == 0) {
    listen(s, 10);
    while (0 == 0) {
      SOCKADDR_IN caddr;
      int clen = sizeof(SOCKADDR_IN);
      printf("Parent: waiting for connection!\n");
      int d = accept(s, (SOCKADDR *)&caddr, &clen);
      printf("Parent: one more client connected!\n");

      // fork() sau accept(): Tao child process xu ly client
      // Child xu ly client, Parent quay lai accept() client moi
      if (fork() == 0) {
        // CHILD PROCESS - Xu ly client

        // File descriptor inheritance: Child ke thua TAT CA file descriptor cua
        // parent Bao gom: server socket (s), client socket (d), stdin, stdout,
        // ... Child KHONG can server socket s -> dong di de giai phong tai
        // nguyen
        close(s);

        while (0 == 0) {
          char buffer[1024] = {0};
          int received = recv(d, buffer, sizeof(buffer) - 1, 0);
          if (received > 0) {
            printf("Child: received %d bytes: %s\n", received, buffer);
          } else {
            printf("Child: disconnected!\n");
            // exit() trong child: Ket thuc child process
            // Quan trong: Phai ket thuc child khi client disconnect
            // Neu khong, child se thanh zombie process
            exit(0);
          }
        }
      }

      // PARENT PROCESS - Tiep tuc accept() client moi
      // Parent KHONG xu ly client -> dong client socket d
      // Neu khong dong, file descriptor leak (het tai nguyen)
      close(d);

      // CHU Y VE ZOMBIE PROCESS:
      // Khi child exit, no tro thanh zombie cho den khi parent goi wait()
      // Trong code nay chua xu ly zombie -> can them signal handler SIGCHLD
    }
  } else {
    printf("Failed to bind!\n");
    close(s);
  }
}