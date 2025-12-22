/*
 * CHUONG TRINH: Multi-thread Command Execution Server
 *
 * KIEN THUC LAP TRINH MANG:
 * - Multi-thread server: Nhieu client ket noi dong thoi
 * - Remote command execution: Thuc thi lenh shell qua mang
 * - Mutex trong network server: Bao ve tai nguyen chung giua cac thread
 */

#include <arpa/inet.h>
#include <errno.h>
#include <malloc.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
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

// Mutex bao ve tai nguyen chung (file out.txt)
// Nhieu thread dong thoi ghi vao 1 file -> ket qua bi xao tron
// Mutex dam bao chi 1 thread ghi tai 1 thoi diem
pthread_mutex_t *pMutex = NULL;

void *ClientThread(void *arg) {
  int c = *((int *)arg);

  while (0 == 0) {
    char buffer[1024] = {0};
    int r = recv(c, buffer, sizeof(buffer) - 1, 0);
    if (r > 0) {
      // Loai bo CRLF tu network protocol
      // Network thuong dung \r\n (Windows style)
      while (buffer[strlen(buffer) - 1] == '\r' ||
             buffer[strlen(buffer) - 1] == '\n') {
        buffer[strlen(buffer) - 1] = 0;
      }

      char command[2048] = {0};
      sprintf(command, "%s > out.txt", buffer);

      // CRITICAL SECTION BAT DAU
      // Tat ca code tu pthread_mutex_lock den pthread_mutex_unlock
      // chi chay boi 1 thread tai 1 thoi diem

      pthread_mutex_lock(pMutex); // Xin quyen truy cap

      // system() + file I/O can bao ve:
      // Thread A chay "ls > out.txt"
      // Thread B chay "pwd > out.txt" -> ghi de ket qua A
      // Thread A doc out.txt -> doc ket qua sai (cua B)
      system(command);

      FILE *f = fopen("out.txt", "rt");
      while (!feof(f)) {
        char line[1024] = {0};
        fgets(line, sizeof(line) - 1, f);
        send(c, line, strlen(line), 0);
      }
      fclose(f);

      pthread_mutex_unlock(pMutex); // Nha quyen truy cap
                                    // CRITICAL SECTION KET THUC
    } else
      break;
  }
  close(c);
  return NULL; // Nen co return cho thread function
}

int main() {
  // Khoi tao mutex TRUOC khi tao thread
  pMutex = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
  pthread_mutex_init(pMutex, NULL);

  SOCKADDR_IN saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = inet_addr("0.0.0.0");
  saddr.sin_port = htons(8888);

  int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (bind(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR)) == 0) {
    listen(s, 10);
    while (0 == 0) {
      int c = accept(s, NULL, 0);
      pthread_t tid;
      int *arg = (int *)calloc(1, sizeof(int));
      *arg = c;
      pthread_create(&tid, NULL, ClientThread, arg);
    }
  } else {
    printf("Failed to bind.\n");
    close(s);
  }

  // Giai phong mutex khi ket thuc
  // Trong truong hop nay khong bao gio chay vi while vô han
  pthread_mutex_destroy(pMutex);
  free(pMutex);
}