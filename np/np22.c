/*
 * CHUONG TRINH: FTP Server - File Transfer Protocol Server
 *
 * KIEN THUC LAP TRINH MANG:
 * - FTP (RFC 959): Giao thuc truyen file co 2 ket noi rieng biet
 * - Control connection (Port 21): Gui nhan lenh FTP
 * - Data connection (Port dong): Truyen file va danh sach thu muc
 * - PASV mode: Passive mode - Server mo port cho client ket noi
 * - FTP commands: USER, PASS, LIST, CWD, PWD, RETR, STOR, PASV, QUIT
 * - Response codes: 220 (Ready), 331 (Need password), 230 (Logged in), etc.
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

int g_clientSockets[1024] = {0};
int g_clientCount = 0;
char rootPath[1024] = {0}; // Current working directory cua client
int dataSocket = -1;       // Socket cho data connection
int pasvPort = 10000;      // Port bat dau cho PASV mode (tang dan)

void Append(char **destination, const char *str) {
  char *tmp = *destination;
  int oldlen = tmp != NULL ? strlen(tmp) : 0;
  int more = strlen(str) + 1;
  tmp = (char *)realloc(tmp, oldlen + more);
  memset(tmp + oldlen, 0, more);
  sprintf(tmp + strlen(tmp), "%s", str);
  *destination = tmp;
}

int Compare(const struct dirent **e1, const struct dirent **e2) {
  if ((*e1)->d_type == (*e2)->d_type) {
    return strcmp((*e1)->d_name, (*e2)->d_name);
  } else {
    if ((*e1)->d_type == DT_DIR)
      return -1;
    else
      return 1;
  }
}

// Tao danh sach file theo format cua FTP LIST command
// Format giong output cua "ls -l" tren Unix
void CreateLIST(const char *path, char **output) {
  struct dirent **result = NULL;

  int n = scandir(path, &result, NULL, Compare);
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      if (result[i]->d_type == DT_DIR) {
        // "d" dau tien = directory
        // rwxrwxrwx = permissions (read/write/execute cho owner/group/others)
        Append(output, "drwxrwxrwx 0 0 0 0 Dec 15 2025 ");
        Append(output, result[i]->d_name);
        Append(output, "\r\n"); // FTP dung CRLF
      } else {
        // "-" dau tien = regular file
        Append(output, "-rwxrwxrwx 0 0 0 0 Dec 15 2025 ");
        Append(output, result[i]->d_name);
        Append(output, "\r\n");
      }
      free(result[i]);
      result[i] = NULL;
    }
  }
}

void Send(int c, char *data, int len) {
  int sent = 0;
  while (sent < len) {
    int tmp = send(c, data + sent, len - sent, 0);
    if (tmp > 0) {
      sent += tmp;
    } else
      break;
  }
}

int main() {
  // Khoi tao current directory = root
  strcpy(rootPath, "/");

  SOCKADDR_IN saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(8888); // FTP chuan dung port 21
  saddr.sin_addr.s_addr = 0;

  int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (bind(s, (SOCKADDR *)&saddr, sizeof(saddr)) == 0) {
    listen(s, 10);
    fd_set read;

    while (0 == 0) {
      FD_ZERO(&read);
      FD_SET(s, &read);
      for (int i = 0; i < g_clientCount; i++) {
        FD_SET(g_clientSockets[i], &read);
      }

      int n = select(FD_SETSIZE, &read, NULL, NULL, NULL);
      if (n > 0) {
        if (FD_ISSET(s, &read)) {
          int c = accept(s, NULL, NULL);
          printf("A new client connected.\n");
          g_clientSockets[g_clientCount++] = c;

          // 220: Service ready for new user
          // FTP response format: "CODE MESSAGE\r\n"
          char *response = "220 MY FTP SERVER\r\n";
          send(c, response, strlen(response), 0);
        }

        for (int i = 0; i < g_clientCount; i++) {
          if (FD_ISSET(g_clientSockets[i], &read)) {
            char data[1024] = {0};
            int c = g_clientSockets[i];
            recv(c, data, sizeof(data) - 1, 0);

            // strncmp(): So sanh n ky tu dau
            // An toan khi kiem tra command prefix

            // USER command: Dang nhap username
            if (strncmp(data, "USER", 4) == 0) {
              // 331: Username OK, need password
              char *response = "331 OK\r\n";
              send(c, response, strlen(response), 0);
            }
            // PASS command: Gui password
            else if (strncmp(data, "PASS", 4) == 0) {
              // 230: User logged in
              char *response = "230 OK\r\n";
              send(c, response, strlen(response), 0);
            }
            // SYST command: Tra ve loai he thong
            else if (strncmp(data, "SYST", 4) == 0) {
              // 215: System type
              char *response = "215 UNIX Type: L8\r\n";
              send(c, response, strlen(response), 0);
            } else if (strncmp(data, "HELP", 4) == 0) {
              // 202: Command not implemented
              char *response = "202 Command not implemented\r\n";
              send(c, response, strlen(response), 0);
            }
            // FEAT command: Tra ve features ho tro
            else if (strncmp(data, "FEAT", 4) == 0) {
              // 211: Features list
              char *response =
                  "211-Features:\r\n EPRT\r\n EPSV\r\n MDTM\r\n PASV\r\n REST "
                  "STREAM\r\n SIZE\r\n TVFS\r\n211 End\r\n";
              send(c, response, strlen(response), 0);
            } else if (strncmp(data, "OPTS", 4) == 0) {
              char *response = "200 OK\r\n";
              send(c, response, strlen(response), 0);
            }
            // PWD command: Print Working Directory
            else if (strncmp(data, "PWD", 3) == 0) {
              char response[2048] = {0};
              // 257: Pathname created (dung cho PWD va MKD)
              sprintf(response, "257 \"%s\" OK\r\n", rootPath);
              send(c, response, strlen(response), 0);
            }
            // TYPE command: Dat kieu truyen (A = ASCII, I = Binary)
            else if (strncmp(data, "TYPE", 4) == 0) {
              char *response = "200 OK\r\n";
              send(c, response, strlen(response), 0);
            }
            // PASV command: Passive mode
            else if (strncmp(data, "PASV", 4) == 0) {
              // PASV response format: 227 OK (h1,h2,h3,h4,p1,p2)
              // IP = h1.h2.h3.h4, Port = p1*256 + p2
              // Vi du: (172,20,38,136,39,16) -> IP 172.20.38.136, Port
              // 39*256+16=10000
              char response[1024] = {0};

              // Bitwise shift de tach port thanh 2 bytes
              // (pasvPort >> 8) & 0xFF: Byte cao (p1)
              // pasvPort & 0xFF: Byte thap (p2)
              sprintf(response, "227 OK (172,20,38,136,%d,%d)\r\n",
                      (pasvPort >> 8) & 0xFF, pasvPort & 0xFF);
              send(c, response, strlen(response), 0);

              // Tao data socket tren pasvPort
              // FTP dung 2 ket noi: control (lenh) va data (file/list)
              SOCKADDR_IN saddr;
              saddr.sin_family = AF_INET;
              saddr.sin_port = htons(pasvPort);
              saddr.sin_addr.s_addr = 0;
              int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

              if (bind(s, (SOCKADDR *)&saddr, sizeof(saddr)) == 0) {
                listen(s, 10);
                // accept() BLOCK cho den khi client ket noi vao data port
                dataSocket = accept(s, NULL, NULL);
                close(s);
                printf("Data connection established.\r\n");
              } else {
                printf("Failed to bind to PASV port.\r\n");
                close(s);
              }

              // Tang port cho lan PASV tiep theo
              // De tranh xung dot port neu client reconnect nhanh
              pasvPort++;
            }
            // LIST command: Liet ke thu muc
            else if (strncmp(data, "LIST", 4) == 0) {
              if (dataSocket > 0) {
                // 150: File status okay; about to open data connection
                char *response1 = "150 OK\r\n";
                send(c, response1, strlen(response1), 0);

                // Gui danh sach file qua DATA connection (khong phai control)
                char *list = NULL;
                CreateLIST(rootPath, &list);
                Send(dataSocket, list, strlen(list));

                // 226: Closing data connection. File transfer successful
                char *response2 = "226 OK\r\n";
                send(c, response2, strlen(response2), 0);

                usleep(100000);
                close(dataSocket);
                dataSocket = -1;
                printf("Data sent.\r\n");
              } else {
                // 425: Can't open data connection
                char *response = "425 Failed to open data connection\r\n";
                send(c, response, strlen(response), 0);
              }
            } else if (strncmp(data, "QUIT", 4) == 0) {
              char *response = "202 Command not implemented\r\n";
              send(c, response, strlen(response), 0);
            }
            // CWD command: Change Working Directory
            else if (strncmp(data, "CWD", 3) == 0) {
              // Parse folder name tu "CWD folder\r\n"
              char *folder = data + 4;

              // Loai bo CRLF cuoi command
              while (folder[strlen(folder) - 1] == '\r' ||
                     folder[strlen(folder) - 1] == '\n') {
                folder[strlen(folder) - 1] = 0;
              }

              // Them folder vao path hien tai
              if (rootPath[strlen(rootPath) - 1] == '/') {
                sprintf(rootPath + strlen(rootPath), "%s", folder);
              } else {
                sprintf(rootPath + strlen(rootPath), "/%s", folder);
              }
              char *response = "200 OK\r\n";
              send(c, response, strlen(response), 0);
            }
            // CDUP command: Change to parent directory
            else if (strncmp(data, "CDUP", 4) == 0) {
              // Di nguoc len thu muc cha: Xoa phan tu '/' cuoi cung
              while (rootPath[strlen(rootPath) - 1] != '/') {
                rootPath[strlen(rootPath) - 1] = 0;
              }
              // Xoa luon '/' cuoi, tru truong hop da o root "/"
              if (strcmp(rootPath, "/") != 0) {
                rootPath[strlen(rootPath) - 1] = 0;
              }
              char *response = "200 OK\r\n";
              send(c, response, strlen(response), 0);
            } else {
              // Command khong ho tro
              char *response = "202 Command not implemented\r\n";
              send(c, response, strlen(response), 0);
            }
          }
        }
      }
    }
  } else
    printf("Failed to bind.\n");

  close(s);
}