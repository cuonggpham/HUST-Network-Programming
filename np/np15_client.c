/*
 * CHUONG TRINH: P2P File Transfer Client - Truyen file truc tiep
 *
 * KIEN THUC LAP TRINH MANG:
 * - Peer-to-peer architecture: Client vua la client vua la server
 * - UDP broadcast de tim client khac trong mang
 * - TCP de truyen file giua 2 client (reliable transfer)
 * - File transfer protocol: Gui kich thuoc file truoc, sau do gui du lieu
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

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

// typedef de tao kieu du lieu moi: STRING la mang char 1024 phan tu
// Giup khai bao mang chuoi de dang hon
typedef char STRING[1024];

// Thong tin cua client nay (hardcoded, nen doc tu config file)
char *MYIP = "172.20.38.136";
int MYPORT = 10000;
char *MYNAME = "UNAME_001";

// Signal handler cho SIGCHLD
void sig_handler(int sig) {
  if (sig == SIGCHLD) {
    int statloc;
    pid_t pid;

    // waitpid(): Thu hoi zombie process
    // -1: Cho bat ky child nao
    // &statloc: Luu exit status cua child
    // WNOHANG: Khong block neu khong co child nao ket thuc
    // Return: PID cua child da ket thuc, 0 neu khong co, -1 neu loi
    // Dung while loop de xu ly nhieu child ket thuc cung luc
    while ((pid = waitpid(-1, &statloc, WNOHANG)) > 0) {
      printf("A child process has been terminated: %d\n", pid);
    }
  }
}

int main() {
  // Dang ky signal handler cho SIGCHLD de tranh zombie process
  signal(SIGCHLD, sig_handler);

  // fork() ngay khi bat dau: Tao 2 process
  // Child: Lang nghe yeu cau SEND tu client khac
  // Parent: Gui REG, LIST, SEND command
  if (fork() == 0) {
    // CHILD PROCESS - Lang nghe yeu cau nhan file
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SOCKADDR_IN saddr, caddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(7000);
    saddr.sin_addr.s_addr = inet_addr("0.0.0.0");

    if (bind(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR)) == 0) {
      while (0 == 0) {
        char buffer[65546] = {0};
        int clen = sizeof(SOCKADDR);
        int received = recvfrom(s, buffer, sizeof(buffer) - 1, 0,
                                (SOCKADDR *)&caddr, &clen);

        // strncmp(): So sanh n ky tu dau cua 2 chuoi
        // Tra ve 0 neu bang, khac 0 neu khac
        // An toan hon strcmp() khi chuoi chua null-terminated
        if (received > 0 && strncmp(buffer, "SEND", 4) == 0) {
          char command[1024] = {0};
          char file[1024] = {0};
          char name[1024] = {0};
          sscanf(buffer, "%s%s%s", command, file, name);

          // strcmp(): So sanh 2 chuoi, tra ve 0 neu BANG
          if (strcmp(name, MYNAME) == 0) {
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "%s %d", MYIP, MYPORT);
            sendto(s, buffer, strlen(buffer), 0, (SOCKADDR *)&caddr, clen);

            // Tao TCP socket de nhan file
            int ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            saddr.sin_family = AF_INET;
            saddr.sin_port = htons(MYPORT);
            saddr.sin_addr.s_addr = inet_addr("0.0.0.0");

            if (bind(ls, (SOCKADDR *)&saddr, sizeof(SOCKADDR)) == 0) {
              listen(ls, 10);
              int ds = accept(ls, NULL, 0);

              // Nhan kich thuoc file truoc (4 bytes = 32-bit int)
              int flen = 0;
              recv(ds, &flen, 4, 0);

              // calloc(): Cap phat va khoi tao bo nho = 0
              // calloc(num_elements, size_each) vs malloc(total_size)
              // calloc an toan hon vi khoi tao = 0
              int bs = 2 * 1024 * 1024;
              char *buffer = (char *)calloc(bs, 1);

              int received = 0;
              FILE *f = fopen("out.dat", "rb");

              // Download file: Nhan cho den khi du kich thuoc
              while (received < flen) {
                int tmp = recv(ds, buffer, sizeof(buffer), 0);
                if (tmp > 0) {
                  fwrite(buffer, tmp, 1, f);
                  received += tmp;
                } else
                  break;
              }
              fclose(f);
              free(buffer);
              close(ds);

              if (received == flen) {
                printf("Received file OK\n");
              } else
                printf("Failed to receive a file\n");
            } else {
              printf("Failed to bind!\n");
              close(ls);
            }
          }
        }
      }
    }
    exit(0);
  }

  // PARENT PROCESS - Gui cac command
  int bs = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  int on = 1;

  // Bat SO_BROADCAST de co the gui broadcast
  setsockopt(bs, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));

  SOCKADDR_IN raddr, laddr, saddr;
  raddr.sin_family = AF_INET;
  raddr.sin_port = htons(5000);
  raddr.sin_addr.s_addr = inet_addr("255.255.255.255");
  laddr.sin_family = AF_INET;
  laddr.sin_port = htons(6000);
  laddr.sin_addr.s_addr = inet_addr("255.255.255.255");
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(7000);
  saddr.sin_addr.s_addr = inet_addr("255.255.255.255");

  while (0 == 0) {
    // Broadcast REG COMMAND (dang ky voi server)
    char buffer[65536] = {0};
    sprintf(buffer, "REG %s", MYNAME);
    sendto(bs, buffer, strlen(buffer), 0, (SOCKADDR *)&raddr, sizeof(SOCKADDR));

    // Doc lenh tu keyboard
    memset(buffer, 0, sizeof(buffer));
    fgets(buffer, sizeof(buffer) - 1, stdin);

    if (strncmp(buffer, "LIST", 4) == 0) {
      sendto(bs, buffer, strlen(buffer), 0, (SOCKADDR *)&laddr,
             sizeof(SOCKADDR));
      memset(buffer, 0, sizeof(buffer));
      int received = recvfrom(bs, buffer, sizeof(buffer) - 1, 0, NULL, 0);
      printf("Received %d bytes from the server.\n", received);
      printf("%s\n", buffer);
    }

    if (strncmp(buffer, "SEND", 4) == 0) {
      char command[1024] = {0};
      char file[1024] = {0};
      char name[1024] = {0};
      sscanf(buffer, "%s%s%s", command, file, name);

      sendto(bs, buffer, strlen(buffer), 0, (SOCKADDR *)&saddr,
             sizeof(SOCKADDR));
      memset(buffer, 0, sizeof(buffer));
      recvfrom(bs, buffer, strlen(buffer) - 1, 0, NULL, 0);

      // usleep(): Sleep trong microseconds (1 giay = 1,000,000 us)
      // Khac voi sleep() tinh bang giay
      usleep(100000); // 100ms

      char ip[1024] = {0};
      int port = 0;
      sscanf(buffer, "%s%d", ip, &port);

      // Ket noi TCP de gui file
      int tcps = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      SOCKADDR_IN taddr;
      taddr.sin_family = AF_INET;
      taddr.sin_port = htons(port);
      taddr.sin_addr.s_addr = inet_addr(ip);

      if (connect(tcps, (SOCKADDR *)&taddr, sizeof(SOCKADDR)) == 0) {
        FILE *f = fopen(file, "rb");

        // fseek() + ftell(): Lay kich thuoc file
        // fseek(file, offset, origin): Di chuyen vi tri doc/ghi
        // SEEK_END: Tinh tu cuoi file, SEEK_SET: Tinh tu dau file
        fseek(f, SEEK_END, 0);

        // ftell(): Tra ve vi tri hien tai (= kich thuoc file khi o cuoi)
        int flen = ftell(f);
        fseek(f, SEEK_SET, 0); // Quay ve dau file

        // Gui kich thuoc file truoc (protocol: size + data)
        send(tcps, &flen, 4, 0);

        int sent = 0;
        int bs = 10 * 1024 * 1024;
        char *data = (char *)calloc(bs, 1);

        // Upload file: Gui cho den khi het du lieu
        while (sent < flen) {
          // fread(): Doc du lieu nhi phan tu file
          // fread(buffer, size_each, count, file)
          int r = fread(data, 1, sizeof(data), f);
          int tmpSent = 0;

          // Vong lap dam bao gui HET du lieu (send co the gui it hon yeu cau)
          while (tmpSent < r) {
            int tmp = send(tcps, data + tmpSent, r - tmpSent, 0);
            if (tmp > 0) {
              tmpSent += tmp;
            } else
              break;
          }
          sent += tmpSent;
        }
        fclose(f);
        free(data);
      } else {
        printf("Failed to connect.\n");
        close(tcps);
      }
    }
  }
}