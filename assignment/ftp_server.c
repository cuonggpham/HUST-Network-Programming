#include <arpa/inet.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>

#define CTRL_PORT 21
#define DATA_PORT 2121
#define BUF_SIZE 1024

char current_dir[512] = "shared";

void send_msg(int sock, const char *msg) { send(sock, msg, strlen(msg), 0); }

void handle_list(int data_sock) {
  DIR *d = opendir(current_dir);
  struct dirent *dir;
  char line[512];

  while ((dir = readdir(d)) != NULL) {
    sprintf(line, "%s\r\n", dir->d_name);
    send(data_sock, line, strlen(line), 0);
  }
  closedir(d);
}

void handle_retr(int data_sock, char *filename) {
  char path[512];
  sprintf(path, "%s/%s", current_dir, filename);

  FILE *f = fopen(path, "rb");
  if (!f)
    return;

  char buf[BUF_SIZE];
  int n;
  while ((n = fread(buf, 1, BUF_SIZE, f)) > 0) {
    send(data_sock, buf, n, 0);
  }
  fclose(f);
}

void handle_stor(int data_sock, char *filename) {
  char path[512];
  sprintf(path, "%s/%s", current_dir, filename);

  FILE *f = fopen(path, "wb");
  if (!f)
    return;

  char buf[BUF_SIZE];
  int n;
  while ((n = recv(data_sock, buf, BUF_SIZE, 0)) > 0) {
    fwrite(buf, 1, n, f);
  }
  fclose(f);
}

int main() {
  int ctrl_sock, client_sock;
  struct sockaddr_in addr;
  char buffer[BUF_SIZE];

  ctrl_sock = socket(AF_INET, SOCK_STREAM, 0);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(CTRL_PORT);

  bind(ctrl_sock, (struct sockaddr *)&addr, sizeof(addr));
  listen(ctrl_sock, 1);

  printf("FTP Server running on port 21...\n");
  client_sock = accept(ctrl_sock, NULL, NULL);

  send_msg(client_sock, "220 Simple FTP Server Ready\r\n");

  while (1) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(client_sock, &fds);

    select(client_sock + 1, &fds, NULL, NULL, NULL);

    if (FD_ISSET(client_sock, &fds)) {
      memset(buffer, 0, BUF_SIZE);
      recv(client_sock, buffer, BUF_SIZE, 0);
      printf("CMD: %s", buffer);

      if (strncmp(buffer, "USER", 4) == 0) {
        send_msg(client_sock, "331 OK\r\n");
      } else if (strncmp(buffer, "PASS", 4) == 0) {
        send_msg(client_sock, "230 Login successful\r\n");
      } else if (strncmp(buffer, "PWD", 3) == 0) {
        char msg[512];
        sprintf(msg, "257 \"%s\"\r\n", current_dir);
        send_msg(client_sock, msg);
      } else if (strncmp(buffer, "CWD", 3) == 0) {
        char dir[256];
        sscanf(buffer + 4, "%s", dir);
        sprintf(current_dir, "shared/%s", dir);
        send_msg(client_sock, "250 Directory changed\r\n");
      } else if (strncmp(buffer, "PASV", 4) == 0) {
        send_msg(client_sock, "227 Entering Passive Mode (127,0,0,1,8,73)\r\n");
      } else if (strncmp(buffer, "LIST", 4) == 0) {
        int data_sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in data_addr;

        data_addr.sin_family = AF_INET;
        data_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        data_addr.sin_port = htons(DATA_PORT);

        connect(data_sock, (struct sockaddr *)&data_addr, sizeof(data_addr));
        send_msg(client_sock, "150 Opening data connection\r\n");

        handle_list(data_sock);
        close(data_sock);
        send_msg(client_sock, "226 Transfer complete\r\n");
      } else if (strncmp(buffer, "RETR", 4) == 0) {
        char file[256];
        sscanf(buffer + 5, "%s", file);

        int data_sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in data_addr;

        data_addr.sin_family = AF_INET;
        data_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        data_addr.sin_port = htons(DATA_PORT);

        connect(data_sock, (struct sockaddr *)&data_addr, sizeof(data_addr));
        send_msg(client_sock, "150 Sending file\r\n");

        handle_retr(data_sock, file);
        close(data_sock);
        send_msg(client_sock, "226 Done\r\n");
      } else if (strncmp(buffer, "STOR", 4) == 0) {
        char file[256];
        sscanf(buffer + 5, "%s", file);

        int data_sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in data_addr;

        data_addr.sin_family = AF_INET;
        data_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        data_addr.sin_port = htons(DATA_PORT);

        connect(data_sock, (struct sockaddr *)&data_addr, sizeof(data_addr));
        send_msg(client_sock, "150 Receiving file\r\n");

        handle_stor(data_sock, file);
        close(data_sock);
        send_msg(client_sock, "226 Upload complete\r\n");
      }
    }
  }
}
