/*
 * CHUONG TRINH: Signal Handling - Inter-Process Communication
 *
 * KIEN THUC LAP TRINH MANG:
 * - Chat server: Broadcast tin nhan tu mot client den tat ca client khac
 * - IPC (Inter-process communication): Child gui signal cho parent
 * - Dung socket noi bo (localhost) de truyen du lieu giua process
 */

#include <arpa/inet.h>
#include <malloc.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Macro de dinh nghia hang so
// SIGRTMIN: Real-time signal dau tien (khac voi standard signal nhu SIGINT,
// SIGCHLD) Real-time signals co the queue (khong bi mat) va co priority
#define MY_SIGNAL SIGRTMIN + 1

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

// Global variables: Truy cap duoc tu moi ham, ke ca signal handler
int g_sockets[1024] = {0}; // Mang luu socket cua cac client
int g_count = 0;           // So luong client hien tai
int s = 0;                 // Server socket
int parentPID = 0;         // PID cua parent process

// Signal handler: Ham duoc goi tu dong khi process nhan signal
// PHAI co signature: void handler(int sig)
// CHU Y: Signal handler nen don gian, tranh goi cac ham khong async-signal-safe
void sig_handler(int sig) {
  // Xu ly SIGINT (Ctrl+C tu terminal)
  // SIGINT = Signal Interrupt, mac dinh ket thuc process
  if (sig == SIGINT) {
    printf("Ctrl+C!\n");
    close(s);
    exit(0);
  }

  // Xu ly SIGCHLD (child process terminated)
  // SIGCHLD duoc kernel gui khi BAT KY child process nao ket thuc
  // Can thiet de tranh zombie process
  if (sig == SIGCHLD) {
    printf("A child process has been terminated!\n");
  }

  // Xu ly custom signal tu child process
  if (sig == MY_SIGNAL) {
    printf("A signal from a child process\n");

    // Tao socket de nhan du lieu tu child (IPC qua localhost)
    int tmp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN tmpAddr;
    tmpAddr.sin_family = AF_INET;
    tmpAddr.sin_port = htons(9999);
    tmpAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // localhost cho IPC

    connect(tmp, (SOCKADDR *)&tmpAddr, sizeof(SOCKADDR));

    char buffer[1024] = {0};
    recv(tmp, buffer, sizeof(buffer) - 1, 0);
    close(tmp);

    // sscanf(): Doc du lieu tu string theo format
    sscanf(buffer, "%d", &tmp);

    // Broadcast message den tat ca client tru nguoi gui
    for (int i = 0; i < g_count; i++) {
      if (g_sockets[i] != tmp) {
        // strstr(): Tim chuoi con, tra ve con tro toi vi tri tim thay
        // +1 de bo qua dau cach, lay phan message
        char *data = strstr(buffer, " ") + 1;
        send(g_sockets[i], data, strlen(data), 0);
      }
    }
  }
}

int main() {
  // getpid(): Lay Process ID cua process hien tai
  // Luu lai de child co the gui signal ve parent
  parentPID = getpid();

  // signal(): Dang ky ham xu ly signal (signal handler)
  // Tham so 1: Signal can xu ly
  // Tham so 2: Con tro toi ham xu ly (signal handler function)
  // Khi process nhan signal, OS se goi ham xu ly thay vi hanh dong mac dinh
  signal(SIGINT, sig_handler);    // Ctrl+C
  signal(SIGCHLD, sig_handler);   // Child terminated
  signal(MY_SIGNAL, sig_handler); // Custom signal

  s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
      g_sockets[g_count++] = d;

      if (fork() == 0) {
        close(s);
        while (0 == 0) {
          char buffer[1024] = {0};
          printf("Child: waiting for data\n");
          int received = recv(d, buffer, sizeof(buffer) - 1, 0);
          if (received > 0) {
            printf("Child: received %d bytes: %s\n", received, buffer);
            int tmp1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            SOCKADDR_IN tmpAddr;
            tmpAddr.sin_family = AF_INET;
            tmpAddr.sin_port = htons(9999);
            tmpAddr.sin_addr.s_addr = 0;
            if (bind(tmp1, (SOCKADDR *)&tmpAddr, sizeof(tmpAddr)) == 0) {
              listen(tmp1, 10);

              // union sigval: Cau truc de truyen them data cung signal
              // Co the chua int hoac pointer
              union sigval v;

              // sigqueue(): Gui signal kem theo data (khac voi kill() chi gui
              // signal) Cho phep truyen thong tin tu child sang parent
              sigqueue(parentPID, MY_SIGNAL, v);

              int tmpLen = sizeof(SOCKADDR);
              int tmp2 = accept(tmp1, (SOCKADDR *)&tmpAddr, &tmpLen);
              char data[2048] = {0};
              sprintf(data, "%d %s", d, buffer);
              send(tmp2, data, strlen(data), 0);
              sleep(1);
              close(tmp2);
              close(tmp1);
              printf("Child: socket closed\n");
            } else
              printf("Child: failed to bind\n");
          } else {
            printf("Child: disconnected!\n");
            exit(0);
          }
        }
      }
    }
  } else {
    printf("Failed to bind!\n");
    close(s);
  }
}