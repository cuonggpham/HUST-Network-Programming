/*
 * CHUONG TRINH: Multi-thread Chat Server - Dung thread de xu ly client
 *
 * KIEN THUC LAP TRINH MANG:
 * - Multi-thread server: Moi client duoc xu ly boi mot thread rieng
 * - Thread: Nhe hon process, dung chung bo nho nen de chia se du lieu
 * - Broadcast message: Gui tin nhan den tat ca client khac
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

// Global variables: Tat ca THREAD deu truy cap duoc (shared memory)
// Khac voi PROCESS: moi process co memory rieng, phai dung IPC
int g_clientSockets[1024] = {0};
int g_clientCount = 0;

// Thread function: Ham chay trong moi thread
// PHAI co signature: void* function(void* arg)
void *ClientThread(void *arg) {
  // Type casting va dereference de lay socket
  // arg la void*, can cast ve int* truoc khi dereference
  int c = *((int *)arg);

  while (0 == 0) {
    char buffer[1024] = {0};
    printf("Waiting for data...\n");

    // recv() trong thread: Chi BLOCK thread hien tai, khong anh huong thread
    // khac Khac voi process: Tat ca process deu doc lap
    int r = recv(c, buffer, sizeof(buffer) - 1, 0);

    if (r > 0) {
      // Broadcast: Gui den TAT CA client dang ket noi
      // Thread truy cap g_clientSockets truc tiep (shared memory)
      // CHU Y: Nen dung mutex khi doc/ghi mang toan cuc
      for (int i = 0; i < g_clientCount; i++) {
        // Khong gui lai cho nguoi gui
        if (g_clientSockets[i] != c) {
          send(g_clientSockets[i], buffer, strlen(buffer), 0);
        }
      }
    } else
      break; // recv = 0: Client dong ket noi, recv < 0: Loi
  }
  printf("A client has disconnected!\n");

  // Thread tu dong ket thuc khi ham return
  // Khong can exit() nhu process
  // Neu dung pthread_exit() thi phai truyen return value
  return NULL;
}

int main() {
  int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  SOCKADDR_IN saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(9999);
  saddr.sin_addr.s_addr = inet_addr("0.0.0.0");

  if (bind(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR)) == 0) {
    listen(s, 10);
    while (0 == 0) {
      printf("Wait for a client...\n");
      int c = accept(s, NULL, 0);

      // Them socket moi vao mang toan cuc TRUOC khi tao thread
      // Thread se doc tu mang nay de broadcast
      g_clientSockets[g_clientCount++] = c;

      pthread_t tid = 0;

      // Cap phat dong cho argument
      // QUAN TRONG: Neu truyen &c (local var), tat ca thread thay cung gia tri
      // Vi c thay doi sau moi accept()
      int *arg = (int *)calloc(1, sizeof(int));
      *arg = c;

      // pthread_create(): Tao thread moi
      // Thread bat dau chay NGAY LAP TUC
      // Khong can pthread_join() vi thread chay doc lap (detached)
      pthread_create(&tid, NULL, ClientThread, (void *)arg);
    }
  } else {
    printf("Failed to bind!\n");
  }
  close(s);
}