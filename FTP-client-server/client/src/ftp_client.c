/*
 * ftp_client.c - Implement xu ly cac lenh FTP cho client
 *
 * KIEN THUC LAP TRINH MANG:
 * - FTP client: Ket noi den server qua control port
 * - PASV mode: Parse response de lay data port
 * - File transfer: Truyen file qua data connection
 */

#include "ftp_client.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Ket noi den FTP server
int ftp_connect(FTPSession *session, const char *host, int port) {
  memset(session, 0, sizeof(FTPSession));
  session->data_socket = -1;

  // Tao socket
  session->ctrl_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (session->ctrl_socket < 0) {
    printf("Khong the tao socket!\n");
    return -1;
  }

  // Phan giai ten mien (neu co)
  struct hostent *he = gethostbyname(host);
  if (he == NULL) {
    printf("Khong the phan giai host: %s\n", host);
    close(session->ctrl_socket);
    return -1;
  }

  // Ket noi
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  memcpy(&server_addr.sin_addr, he->h_addr, he->h_length);

  strncpy(session->server_ip, inet_ntoa(server_addr.sin_addr),
          sizeof(session->server_ip) - 1);
  session->server_port = port;

  if (connect(session->ctrl_socket, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) < 0) {
    printf("Khong the ket noi den server!\n");
    close(session->ctrl_socket);
    return -1;
  }

  // Nhan thong bao chao mung
  char buffer[BUFFER_SIZE];
  ftp_recv_response(session, buffer, sizeof(buffer));

  return 0;
}

// Ngat ket noi
void ftp_disconnect(FTPSession *session) {
  if (session->data_socket > 0) {
    close(session->data_socket);
    session->data_socket = -1;
  }
  if (session->ctrl_socket > 0) {
    close(session->ctrl_socket);
    session->ctrl_socket = -1;
  }
}

// Gui lenh den server
int ftp_send_command(FTPSession *session, const char *cmd) {
  char buffer[BUFFER_SIZE];
  snprintf(buffer, sizeof(buffer), "%s\r\n", cmd);
  return send(session->ctrl_socket, buffer, strlen(buffer), 0);
}

// Nhan phan hoi tu server
int ftp_recv_response(FTPSession *session, char *buffer, int size) {
  memset(buffer, 0, size);
  int received = recv(session->ctrl_socket, buffer, size - 1, 0);

  if (received <= 0) {
    return -1;
  }

  // Parse response code (3 ky tu dau)
  int code = 0;
  sscanf(buffer, "%d", &code);

  return code;
}

// Gui lenh va doi phan hoi
int ftp_execute(FTPSession *session, const char *cmd, char *response,
                int size) {
  ftp_send_command(session, cmd);
  return ftp_recv_response(session, response, size);
}

// Dang nhap
int ftp_login(FTPSession *session, const char *username, const char *password) {
  char cmd[256];
  char response[BUFFER_SIZE];
  int code;

  // Gui USER
  snprintf(cmd, sizeof(cmd), "USER %s", username);
  code = ftp_execute(session, cmd, response, sizeof(response));

  if (code != 331 && code != 230) {
    return -1;
  }

  // Gui PASS (neu can)
  if (code == 331) {
    snprintf(cmd, sizeof(cmd), "PASS %s", password);
    code = ftp_execute(session, cmd, response, sizeof(response));

    if (code != 230) {
      return -1;
    }
  }

  session->logged_in = 1;
  return 0;
}

// Lay thu muc hien tai
int ftp_pwd(FTPSession *session, char *path) {
  char response[BUFFER_SIZE];
  int code = ftp_execute(session, "PWD", response, sizeof(response));

  if (code == 257) {
    // Parse thu muc tu response: 257 "/path" is current directory
    char *start = strchr(response, '"');
    if (start != NULL) {
      start++;
      char *end = strchr(start, '"');
      if (end != NULL) {
        int len = end - start;
        strncpy(path, start, len);
        path[len] = '\0';
        return 0;
      }
    }
  }

  return -1;
}

// Doi thu muc
int ftp_cwd(FTPSession *session, const char *path) {
  char cmd[512];
  char response[BUFFER_SIZE];

  snprintf(cmd, sizeof(cmd), "CWD %s", path);
  int code = ftp_execute(session, cmd, response, sizeof(response));

  return (code == 250) ? 0 : -1;
}

// Thiet lap PASV mode
int ftp_pasv(FTPSession *session) {
  char response[BUFFER_SIZE];
  int code = ftp_execute(session, "PASV", response, sizeof(response));

  if (code != 227) {
    printf("PASV failed: %s\n", response);
    return -1;
  }

  // Parse IP va port tu response: 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)
  int h1, h2, h3, h4, p1, p2;
  char *start = strchr(response, '(');
  if (start == NULL) {
    printf("Invalid PASV response\n");
    return -1;
  }

  if (sscanf(start, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2) != 6) {
    printf("Cannot parse PASV response\n");
    return -1;
  }

  // Tinh port va IP
  int data_port = p1 * 256 + p2;
  char data_ip[32];
  snprintf(data_ip, sizeof(data_ip), "%d.%d.%d.%d", h1, h2, h3, h4);

  // Ket noi den data port
  session->data_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (session->data_socket < 0) {
    return -1;
  }

  struct sockaddr_in data_addr;
  data_addr.sin_family = AF_INET;
  data_addr.sin_port = htons(data_port);
  data_addr.sin_addr.s_addr = inet_addr(data_ip);

  if (connect(session->data_socket, (struct sockaddr *)&data_addr,
              sizeof(data_addr)) < 0) {
    printf("Khong the ket noi data port %s:%d\n", data_ip, data_port);
    close(session->data_socket);
    session->data_socket = -1;
    return -1;
  }

  return session->data_socket;
}

// Liet ke thu muc
int ftp_list(FTPSession *session) {
  // Thiet lap PASV truoc
  if (ftp_pasv(session) < 0) {
    return -1;
  }

  // Gui lenh LIST
  char response[BUFFER_SIZE];
  ftp_send_command(session, "LIST");
  int code = ftp_recv_response(session, response, sizeof(response));

  if (code != 150 && code != 125) {
    close(session->data_socket);
    session->data_socket = -1;
    return -1;
  }

  // Nhan du lieu tu data connection
  char data[BUFFER_SIZE * 4];
  int total = 0;
  int received;

  while ((received = recv(session->data_socket, data + total,
                          sizeof(data) - total - 1, 0)) > 0) {
    total += received;
  }
  data[total] = '\0';

  // In danh sach
  printf("%s", data);

  close(session->data_socket);
  session->data_socket = -1;

  // Nhan response ket thuc
  ftp_recv_response(session, response, sizeof(response));

  return 0;
}

// Download file
int ftp_download(FTPSession *session, const char *remote_file,
                 const char *local_file) {
  // Thiet lap PASV
  if (ftp_pasv(session) < 0) {
    return -1;
  }

  // Gui lenh RETR
  char cmd[512];
  char response[BUFFER_SIZE];

  snprintf(cmd, sizeof(cmd), "RETR %s", remote_file);
  ftp_send_command(session, cmd);
  int code = ftp_recv_response(session, response, sizeof(response));

  if (code != 150 && code != 125) {
    printf("RETR failed: %s\n", response);
    close(session->data_socket);
    session->data_socket = -1;
    return -1;
  }

  // Mo file local de ghi
  FILE *f = fopen(local_file, "wb");
  if (f == NULL) {
    printf("Khong the tao file: %s\n", local_file);
    close(session->data_socket);
    session->data_socket = -1;
    return -1;
  }

  // Nhan du lieu va ghi file
  char buffer[BUFFER_SIZE];
  int received;
  int total = 0;

  while ((received = recv(session->data_socket, buffer, sizeof(buffer), 0)) >
         0) {
    fwrite(buffer, 1, received, f);
    total += received;
  }

  fclose(f);
  close(session->data_socket);
  session->data_socket = -1;

  printf("Da download %d bytes vao %s\n", total, local_file);

  // Nhan response ket thuc
  ftp_recv_response(session, response, sizeof(response));

  return 0;
}

// Upload file
int ftp_upload(FTPSession *session, const char *local_file,
               const char *remote_file) {
  // Mo file local de doc
  FILE *f = fopen(local_file, "rb");
  if (f == NULL) {
    printf("Khong the mo file: %s\n", local_file);
    return -1;
  }

  // Thiet lap PASV
  if (ftp_pasv(session) < 0) {
    fclose(f);
    return -1;
  }

  // Gui lenh STOR
  char cmd[512];
  char response[BUFFER_SIZE];

  snprintf(cmd, sizeof(cmd), "STOR %s", remote_file);
  ftp_send_command(session, cmd);
  int code = ftp_recv_response(session, response, sizeof(response));

  if (code != 150 && code != 125) {
    printf("STOR failed: %s\n", response);
    fclose(f);
    close(session->data_socket);
    session->data_socket = -1;
    return -1;
  }

  // Doc file va gui
  char buffer[BUFFER_SIZE];
  int bytes_read;
  int total = 0;

  while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
    int sent = 0;
    while (sent < bytes_read) {
      int n = send(session->data_socket, buffer + sent, bytes_read - sent, 0);
      if (n <= 0)
        break;
      sent += n;
    }
    total += sent;
  }

  fclose(f);
  close(session->data_socket);
  session->data_socket = -1;

  printf("Da upload %d bytes tu %s\n", total, local_file);

  // Nhan response ket thuc
  ftp_recv_response(session, response, sizeof(response));

  return 0;
}

// Ngat ket noi
int ftp_quit(FTPSession *session) {
  char response[BUFFER_SIZE];
  ftp_execute(session, "QUIT", response, sizeof(response));
  ftp_disconnect(session);
  return 0;
}