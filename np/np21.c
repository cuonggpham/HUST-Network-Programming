/*
 * CHUONG TRINH: I/O Multiplexing voi select() - Xu ly nhieu socket cung luc
 *
 * KIEN THUC LAP TRINH MANG:
 * - select(): Cho doi su kien tren nhieu socket dong thoi
 * - I/O Multiplexing: Mot thread xu ly nhieu ket noi (khong can fork/pthread)
 * - fd_set: Tap hop cac file descriptor de theo doi
 * - Uu diem: Khong ton overhead tao thread/process cho moi client
 */

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <malloc.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

int g_clientSockets[1024] = {0}; // Mang luu tat ca client socket
int g_clientCount = 0;

int main() {
  SOCKADDR_IN saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(8888);
  saddr.sin_addr.s_addr = 0;

  int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (bind(s, (SOCKADDR *)&saddr, sizeof(saddr)) == 0) {
    listen(s, 10);

    // fd_set: Kieu du lieu tap hop file descriptor
    // Thuc chat la bit array, moi bit dai dien 1 file descriptor
    fd_set read;

    while (0 == 0) {
      // FD_ZERO(): Macro xoa TAT CA FD khoi set
      // Phai goi truoc moi vong lap select() vi select() sua doi set
      FD_ZERO(&read);

      // FD_SET(): Macro them FD vao set
      // Them server socket de theo doi ket noi moi
      FD_SET(s, &read);

      // Them tat ca client socket vao set de theo doi du lieu
      for (int i = 0; i < g_clientCount; i++) {
        FD_SET(g_clientSockets[i], &read);
      }

      // select(): System call cho doi su kien tren nhieu FD
      // Tham so 1: nfds = FD lon nhat + 1 (dung FD_SETSIZE de don gian)
      // Tham so 2: readfds - FD can theo doi doc (co du lieu den)
      // Tham so 3: writefds - FD can theo doi ghi (buffer trong, co the ghi)
      // Tham so 4: exceptfds - FD can theo doi exception
      // Tham so 5: timeout - NULL = block vo han, 0 = poll (khong block)
      // Return: So FD san sang, 0 neu timeout, -1 neu loi
      //
      // SAU select(), fd_set CHI CON cac FD san sang (bi modify)
      // Do do phai FD_ZERO va FD_SET lai moi vong lap
      int n = select(FD_SETSIZE, &read, NULL, NULL, NULL);

      if (n > 0) // Co it nhat 1 FD san sang
      {
        // FD_ISSET(): Macro kiem tra FD co trong set khong
        // Truoc select(): FD nao ta them vao
        // Sau select(): FD nao san sang (co su kien)
        if (FD_ISSET(s, &read)) {
          // Server socket san sang -> co client moi ket noi
          int c = accept(s, NULL, NULL);
          printf("A new client connected.\n");
          g_clientSockets[g_clientCount++] = c;
        }

        // Kiem tra client socket nao co du lieu
        for (int i = 0; i < g_clientCount; i++) {
          if (FD_ISSET(g_clientSockets[i], &read)) {
            // Client socket san sang -> co du lieu gui den
            char data[1024] = {0};
            recv(g_clientSockets[i], data, sizeof(data) - 1, 0);

            // Broadcast den cac client khac
            for (int j = 0; j < g_clientCount; j++) {
              if (j != i)
                send(g_clientSockets[j], data, strlen(data), 0);
            }
          }
        }
      }
    }
  } else
    printf("Failed to bind.\n");

  close(s);
}