/*
 * ftp_server.c - Implement xu ly cac lenh FTP
 *
 * Xu ly cac lenh FTP co ban theo RFC 959
 * Server ho tro PASV mode de tuong thich voi FileZilla
 */

#include "ftp_server.h"
#include "account.h"
#include <arpa/inet.h>
#include <dirent.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// Gui response den client
void send_response(int socket, const char *response) {
  send(socket, response, strlen(response), 0);
}

// Lay duong dan tuyet doi
void get_absolute_path(ClientSession *session, char *abs_path) {
  if (session->current_dir[0] == '/') {
    sprintf(abs_path, "%s%s", session->root_dir, session->current_dir);
  } else {
    sprintf(abs_path, "%s/%s", session->root_dir, session->current_dir);
  }

  // Bo dau / thua o cuoi
  int len = strlen(abs_path);
  if (len > 1 && abs_path[len - 1] == '/') {
    abs_path[len - 1] = '\0';
  }
}

// Xu ly lenh USER
void handle_USER(ClientSession *session, const char *username) {
  strncpy(session->username, username, sizeof(session->username) - 1);
  session->logged_in = 0; // Chua dang nhap, can password

  char response[128];
  sprintf(response, "331 Password required for %s\r\n", username);
  send_response(session->ctrl_socket, response);
}

// Xu ly lenh PASS
void handle_PASS(ClientSession *session, const char *password) {
  int result = validate_login(session->username, password);

  if (result >= 0) {
    session->logged_in = 1;

    // Lay thu muc goc cua user
    const char *root = get_user_root_dir(session->username);
    if (root != NULL) {
      strncpy(session->root_dir, root, sizeof(session->root_dir) - 1);
    } else {
      strcpy(session->root_dir, "/tmp"); // Mac dinh
    }
    strcpy(session->current_dir, "/");

    char response[128];
    sprintf(response, "230 User %s logged in\r\n", session->username);
    send_response(session->ctrl_socket, response);
  } else {
    send_response(session->ctrl_socket, "530 Login incorrect\r\n");
  }
}

// Xu ly lenh SYST
void handle_SYST(ClientSession *session) {
  send_response(session->ctrl_socket, "215 UNIX Type: L8\r\n");
}

// Xu ly lenh FEAT
void handle_FEAT(ClientSession *session) {
  char *response = "211-Features:\r\n"
                   " PASV\r\n"
                   " SIZE\r\n"
                   " MDTM\r\n"
                   "211 End\r\n";
  send_response(session->ctrl_socket, response);
}

// Xu ly lenh PWD
void handle_PWD(ClientSession *session) {
  if (!session->logged_in) {
    send_response(session->ctrl_socket, "530 Please login first\r\n");
    return;
  }

  char response[512];
  sprintf(response, "257 \"%s\" is current directory\r\n",
          session->current_dir);
  send_response(session->ctrl_socket, response);
}

// Xu ly lenh CWD
void handle_CWD(ClientSession *session, const char *path) {
  if (!session->logged_in) {
    send_response(session->ctrl_socket, "530 Please login first\r\n");
    return;
  }

  char new_path[MAX_PATH];

  if (path[0] == '/') {
    // Duong dan tuyet doi
    strncpy(new_path, path, sizeof(new_path) - 1);
  } else {
    // Duong dan tuong doi
    if (strcmp(session->current_dir, "/") == 0) {
      sprintf(new_path, "/%s", path);
    } else {
      sprintf(new_path, "%s/%s", session->current_dir, path);
    }
  }

  // Kiem tra thu muc co ton tai khong
  char abs_path[MAX_PATH * 2];
  sprintf(abs_path, "%s%s", session->root_dir, new_path);

  struct stat st;
  if (stat(abs_path, &st) == 0 && S_ISDIR(st.st_mode)) {
    strncpy(session->current_dir, new_path, sizeof(session->current_dir) - 1);
    send_response(session->ctrl_socket, "250 Directory changed\r\n");
  } else {
    send_response(session->ctrl_socket, "550 Directory not found\r\n");
  }
}

// Xu ly lenh CDUP
void handle_CDUP(ClientSession *session) {
  if (!session->logged_in) {
    send_response(session->ctrl_socket, "530 Please login first\r\n");
    return;
  }

  // Tim dau / cuoi cung va cat bo
  char *last_slash = strrchr(session->current_dir, '/');
  if (last_slash != session->current_dir && last_slash != NULL) {
    *last_slash = '\0';
  } else {
    strcpy(session->current_dir, "/");
  }

  send_response(session->ctrl_socket, "250 Directory changed to parent\r\n");
}

// Xu ly lenh TYPE
void handle_TYPE(ClientSession *session, const char *type) {
  // Chap nhan ca ASCII va Binary
  send_response(session->ctrl_socket, "200 Type set\r\n");
}

// Xu ly lenh PASV
void handle_PASV(ClientSession *session, const char *server_ip) {
  if (!session->logged_in) {
    send_response(session->ctrl_socket, "530 Please login first\r\n");
    return;
  }

  // Dong socket PASV cu neu con mo
  if (session->pasv_listen_socket > 0) {
    close(session->pasv_listen_socket);
    session->pasv_listen_socket = -1;
  }

  // Tao socket moi cho data connection
  int data_listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (data_listen_sock < 0) {
    send_response(session->ctrl_socket, "425 Cannot open data connection\r\n");
    return;
  }

  // Cho phep reuse address
  int reuse = 1;
  setsockopt(data_listen_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  struct sockaddr_in data_addr;
  data_addr.sin_family = AF_INET;
  data_addr.sin_addr.s_addr = INADDR_ANY;
  data_addr.sin_port = htons(session->pasv_port);

  if (bind(data_listen_sock, (struct sockaddr *)&data_addr, sizeof(data_addr)) <
      0) {
    close(data_listen_sock);
    send_response(session->ctrl_socket, "425 Cannot open data connection\r\n");
    return;
  }

  listen(data_listen_sock, 1);
  session->pasv_listen_socket = data_listen_sock;

  // Parse IP thanh 4 so
  int ip1, ip2, ip3, ip4;
  sscanf(server_ip, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);

  // Tinh p1 va p2 tu port
  int p1 = (session->pasv_port >> 8) & 0xFF;
  int p2 = session->pasv_port & 0xFF;

  char response[128];
  sprintf(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n", ip1,
          ip2, ip3, ip4, p1, p2);
  send_response(session->ctrl_socket, response);

  // Tang port cho lan sau
  session->pasv_port++;
  if (session->pasv_port > 65000) {
    session->pasv_port = 10000;
  }
}

// Xu ly lenh LIST
void handle_LIST(ClientSession *session) {
  if (!session->logged_in) {
    send_response(session->ctrl_socket, "530 Please login first\r\n");
    return;
  }

  if (session->pasv_listen_socket <= 0) {
    send_response(session->ctrl_socket, "425 Use PASV first\r\n");
    return;
  }

  // Chap nhan ket noi data tu client
  session->data_socket = accept(session->pasv_listen_socket, NULL, NULL);
  close(session->pasv_listen_socket);
  session->pasv_listen_socket = -1;

  if (session->data_socket < 0) {
    send_response(session->ctrl_socket, "425 Cannot open data connection\r\n");
    return;
  }

  send_response(session->ctrl_socket,
                "150 Opening data connection for LIST\r\n");

  // Lay danh sach file
  char abs_path[MAX_PATH * 2];
  get_absolute_path(session, abs_path);

  DIR *dir = opendir(abs_path);
  if (dir == NULL) {
    send_response(session->ctrl_socket, "550 Cannot open directory\r\n");
    close(session->data_socket);
    session->data_socket = -1;
    return;
  }

  char list_buffer[8192] = {0};
  struct dirent *entry;

  while ((entry = readdir(dir)) != NULL) {
    // Bo qua . va ..
    // if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
    // {
    //     continue;
    // }

    // Lay thong tin file
    char filepath[MAX_PATH * 2 + 256];
    sprintf(filepath, "%s/%s", abs_path, entry->d_name);

    struct stat st;
    if (stat(filepath, &st) == 0) {
      char line[512];
      char time_str[32];
      struct tm *tm = localtime(&st.st_mtime);
      strftime(time_str, sizeof(time_str), "%b %d %H:%M", tm);

      if (S_ISDIR(st.st_mode)) {
        sprintf(line, "drwxr-xr-x 1 ftp ftp %10ld %s %s\r\n", (long)st.st_size,
                time_str, entry->d_name);
      } else {
        sprintf(line, "-rw-r--r-- 1 ftp ftp %10ld %s %s\r\n", (long)st.st_size,
                time_str, entry->d_name);
      }
      strcat(list_buffer, line);
    }
  }

  closedir(dir);

  // Gui danh sach qua data connection
  send(session->data_socket, list_buffer, strlen(list_buffer), 0);

  close(session->data_socket);
  session->data_socket = -1;

  send_response(session->ctrl_socket, "226 Transfer complete\r\n");
}

// Xu ly lenh RETR (download file)
void handle_RETR(ClientSession *session, const char *filename) {
  if (!session->logged_in) {
    send_response(session->ctrl_socket, "530 Please login first\r\n");
    return;
  }

  if (session->pasv_listen_socket <= 0) {
    send_response(session->ctrl_socket, "425 Use PASV first\r\n");
    return;
  }

  // Tao duong dan day du
  char filepath[MAX_PATH * 2];
  get_absolute_path(session, filepath);
  strcat(filepath, "/");
  strcat(filepath, filename);

  // Mo file
  FILE *f = fopen(filepath, "rb");
  if (f == NULL) {
    send_response(session->ctrl_socket, "550 File not found\r\n");
    return;
  }

  // Chap nhan ket noi data
  session->data_socket = accept(session->pasv_listen_socket, NULL, NULL);
  close(session->pasv_listen_socket);
  session->pasv_listen_socket = -1;

  if (session->data_socket < 0) {
    fclose(f);
    send_response(session->ctrl_socket, "425 Cannot open data connection\r\n");
    return;
  }

  send_response(session->ctrl_socket,
                "150 Opening data connection for file transfer\r\n");

  // Gui file
  char buffer[BUFFER_SIZE];
  int bytes_read;

  while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
    int sent = 0;
    while (sent < bytes_read) {
      int n = send(session->data_socket, buffer + sent, bytes_read - sent, 0);
      if (n <= 0)
        break;
      sent += n;
    }
  }

  fclose(f);
  close(session->data_socket);
  session->data_socket = -1;

  send_response(session->ctrl_socket, "226 Transfer complete\r\n");
}

// Xu ly lenh STOR (upload file)
void handle_STOR(ClientSession *session, const char *filename) {
  if (!session->logged_in) {
    send_response(session->ctrl_socket, "530 Please login first\r\n");
    return;
  }

  if (session->pasv_listen_socket <= 0) {
    send_response(session->ctrl_socket, "425 Use PASV first\r\n");
    return;
  }

  // Tao duong dan day du
  char filepath[MAX_PATH * 2];
  get_absolute_path(session, filepath);
  strcat(filepath, "/");
  strcat(filepath, filename);

  // Chap nhan ket noi data
  session->data_socket = accept(session->pasv_listen_socket, NULL, NULL);
  close(session->pasv_listen_socket);
  session->pasv_listen_socket = -1;

  if (session->data_socket < 0) {
    send_response(session->ctrl_socket, "425 Cannot open data connection\r\n");
    return;
  }

  // Mo file de ghi
  FILE *f = fopen(filepath, "wb");
  if (f == NULL) {
    send_response(session->ctrl_socket, "550 Cannot create file\r\n");
    close(session->data_socket);
    session->data_socket = -1;
    return;
  }

  send_response(session->ctrl_socket,
                "150 Opening data connection for file upload\r\n");

  // Nhan du lieu va ghi file
  char buffer[BUFFER_SIZE];
  int bytes_received;

  while ((bytes_received =
              recv(session->data_socket, buffer, sizeof(buffer), 0)) > 0) {
    fwrite(buffer, 1, bytes_received, f);
  }

  fclose(f);
  close(session->data_socket);
  session->data_socket = -1;

  send_response(session->ctrl_socket, "226 Transfer complete\r\n");
}

// Xu ly lenh QUIT
void handle_QUIT(ClientSession *session) {
  send_response(session->ctrl_socket, "221 Goodbye\r\n");
  session->logged_in = 0;
}