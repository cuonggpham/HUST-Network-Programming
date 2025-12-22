/*
 * CHUONG TRINH: P2P Directory Server - Quan ly danh sach client
 *
 * KIEN THUC LAP TRINH MANG:
 * - Directory server: Luu thong tin cac peer trong mang P2P
 * - Registration: Client dang ky voi server bang UDP broadcast
 * - List command: Tra ve danh sach tat ca client dang online
 */

#include <arpa/inet.h>
#include <errno.h>
#include <malloc.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MY_SIGNAL SIGRTMIN + 1

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

// typedef mang ky tu: Tao kieu STRING la char[1024]
// Khai bao "STRING name" tuong duong "char name[1024]"
typedef char STRING[1024];

int parentPID = 0;

// Mang toan cuc luu ten cac client da dang ky
STRING g_clients[1024] = {0};
int g_count = 0;

void sig_handler(int sig) {
  if (sig == SIGCHLD) {
    int statloc;
    pid_t pid;
    // waitpid voi WNOHANG: Khong block, tra ve ngay
    while ((pid = waitpid(-1, &statloc, WNOHANG)) > 0) {
      printf("A child process has been terminated: %d\n", pid);
    }
  }
  if (sig == MY_SIGNAL) {
    printf("Got a signal from a child process!\n");

    // usleep(): Delay trong microseconds
    // Can thiet de cho child process san sang nhan ket noi
    usleep(100000);

    // Tao chuoi response voi format: "LIST count name1 name2 ..."
    char buffer[65536] = {0};
    sprintf(buffer, "LIST %d", g_count);
    for (int i = 0; i < g_count; i++) {
      // sprintf vao cuoi chuoi: sprintf(str + strlen(str), ...)
      sprintf(buffer + strlen(buffer), " %s", g_clients[i]);
    }

    // Gui response qua localhost socket (IPC)
    SOCKADDR_IN caddr;
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(8000);
    caddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    int c = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sendto(c, buffer, strlen(buffer), 0, (SOCKADDR *)&caddr, sizeof(SOCKADDR));
    close(c);
  }
}

int main() {
  parentPID = getpid();
  signal(SIGCHLD, sig_handler);
  signal(MY_SIGNAL, sig_handler);

  int s1 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  SOCKADDR_IN saddr1;
  saddr1.sin_family = AF_INET;
  saddr1.sin_port = htons(5000);
  saddr1.sin_addr.s_addr = inet_addr("0.0.0.0");

  if (bind(s1, (SOCKADDR *)&saddr1, sizeof(SOCKADDR)) == 0) {
    // Tao child process xu ly LIST command
    if (fork() == 0) {
      int s2 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      SOCKADDR_IN saddr2;
      saddr2.sin_family = AF_INET;
      saddr2.sin_port = htons(6000);
      saddr2.sin_addr.s_addr = inet_addr("0.0.0.0");

      if (bind(s2, (SOCKADDR *)&saddr2, sizeof(SOCKADDR)) == 0) {
        while (0 == 0) {
          SOCKADDR_IN caddr;
          int clen = sizeof(SOCKADDR_IN);
          char buffer[65536] = {0};
          int received = recvfrom(s2, buffer, sizeof(buffer) - 1, 0,
                                  (SOCKADDR *)&caddr, &clen);

          if (received > 0 && strncmp(buffer, "LIST", 4) == 0) {
            printf("Received a LIST command.\n");

            // Gui signal cho parent de lay danh sach client
            // Child khong co quyen truy cap g_clients (copy rieng sau fork)
            union sigval v;
            sigqueue(parentPID, MY_SIGNAL, v);

            // Cho nhan response tu parent qua localhost
            int s3 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            SOCKADDR_IN saddr3;
            saddr3.sin_port = htons(8000);
            saddr3.sin_family = AF_INET;
            saddr3.sin_addr.s_addr = inet_addr("127.0.0.1");

            if (bind(s3, (SOCKADDR *)&saddr3, sizeof(SOCKADDR)) == 0) {
              memset(buffer, 0, sizeof(buffer));
              received = recvfrom(s3, buffer, sizeof(buffer) - 1, 0, NULL, 0);
              if (received > 0) {
                printf("Replied with: %s.\n", buffer);
                // Chuyen tiep response cho client goc
                sendto(s2, buffer, strlen(buffer), 0, (SOCKADDR *)&caddr, clen);
              }
              close(s3);
            } else {
              printf("Failed to bind to port 8000!\n");
              close(s3);
              close(s2);
              exit(0);
            }
          }
        }
      } else {
        printf("Failed to bind to port 6000!\n");
        close(s2);
      }
      exit(0);
    }

    // PARENT: Xu ly REG command (dang ky client moi)
    while (0 == 0) {
      SOCKADDR_IN caddr;
      int clen = sizeof(SOCKADDR_IN);
      char buffer[65536] = {0};
      int received = recvfrom(s1, buffer, sizeof(buffer) - 1, 0,
                              (SOCKADDR *)&caddr, &clen);

      // strncmp(s1, s2, n): Chi so sanh n ky tu dau
      // "REG " co 4 ky tu (bao gom dau cach)
      if (received > 0 && strncmp(buffer, "REG ", 4) == 0) {
        char tmp[1024] = {0};
        // buffer + 4: Bo qua "REG ", doc ten client
        sscanf(buffer + 4, "%s", tmp);

        // Kiem tra client da ton tai chua
        int i = 0;
        for (i = 0; i < g_count; i++) {
          if (strcmp(g_clients[i], tmp) == 0) {
            break;
          }
        }

        // Neu chua co thi them moi
        if (i == g_count) {
          // strcpy(): Copy chuoi (luon nho check destination du lon)
          strcpy(g_clients[g_count], tmp);
          g_count++;
          printf("Added a new client with name: %s\n", tmp);
        }
      }
    }
  } else {
    printf("Failed to bind to port 5000!\n");
    close(s1);
  }
}
