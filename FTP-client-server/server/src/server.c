/*
 * server.c - FTP Server chinh
 *
 * KIEN THUC LAP TRINH MANG:
 * - FTP protocol: Giao thuc truyen file, control port 21/2121
 * - Multi-threading: Moi client mot thread
 * - Passive mode: Server mo port cho client ket noi data
 *
 * KIEN THUC C ADVANCE:
 * - pthread: Tao thread moi cho moi client
 * - Signal handling: Xu ly Ctrl+C de tat server
 * - Log theo dinh dang: hh:mm:ss <Lenh> <IP client>
 */

#include "account.h"
#include "ftp_server.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define FTP_PORT 2121 // Port FTP (dung 2121 thay vi 21 de khong can root)
#define ACCOUNTS_FILE "../data/accounts.dat"

// Socket server toan cuc de dong khi Ctrl+C
int g_server_socket = -1;

// Bien dem port cho PASV mode (moi client bat dau tu port khac nhau)
int g_pasv_port_counter = 10000;
pthread_mutex_t g_port_mutex = PTHREAD_MUTEX_INITIALIZER;

// IP cua server (can thay doi cho phu hop voi mang cua ban)
char g_server_ip[32] = "127.0.0.1";

// Lay thoi gian hien tai dang hh:mm:ss
void get_time_string(char *buffer, int size) {
  time_t now = time(NULL);
  struct tm *tm = localtime(&now);
  strftime(buffer, size, "%H:%M:%S", tm);
}

// Log lenh nhan duoc
void log_command(const char *command, const char *client_ip) {
  char time_str[16];
  get_time_string(time_str, sizeof(time_str));
  printf("%s %s %s\n", time_str, command, client_ip);
}

// Xu ly signal Ctrl+C
void signal_handler(int sig) {
  if (sig == SIGINT) {
    printf("\nDang tat server...\n");
    if (g_server_socket > 0) {
      close(g_server_socket);
    }
    exit(0);
  }
}

// Ham xu ly mot client trong thread rieng
void *client_thread(void *arg) {
  ClientSession session;
  memset(&session, 0, sizeof(session));

  session.ctrl_socket = *((int *)arg);
  free(arg);

  // Lay port cho PASV mode
  pthread_mutex_lock(&g_port_mutex);
  session.pasv_port = g_pasv_port_counter;
  g_pasv_port_counter += 100; // Moi client duoc 100 port cho nhieu lan PASV
  if (g_pasv_port_counter > 60000) {
    g_pasv_port_counter = 10000;
  }
  pthread_mutex_unlock(&g_port_mutex);

  session.data_socket = -1;
  session.pasv_listen_socket = -1;
  session.logged_in = 0;
  strcpy(session.current_dir, "/");

  // Lay IP client
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);
  getpeername(session.ctrl_socket, (struct sockaddr *)&client_addr, &addr_len);
  strncpy(session.client_ip, inet_ntoa(client_addr.sin_addr),
          sizeof(session.client_ip) - 1);

  // Gui thong bao chao mung
  send_response(session.ctrl_socket, "220 Simple FTP Server Ready\r\n");

  char buffer[BUFFER_SIZE];

  while (1) {
    memset(buffer, 0, sizeof(buffer));
    int received = recv(session.ctrl_socket, buffer, sizeof(buffer) - 1, 0);

    if (received <= 0) {
      printf("Client %s da ngat ket noi\n", session.client_ip);
      break;
    }

    // Bo ky tu xuong dong
    buffer[strcspn(buffer, "\r\n")] = 0;

    // Log lenh
    log_command(buffer, session.client_ip);

    // Parse lenh va tham so
    char command[16] = {0};
    char param[BUFFER_SIZE] = {0};
    sscanf(buffer, "%s %[^\n]", command, param);

    // Xu ly cac lenh FTP
    if (strcasecmp(command, "USER") == 0) {
      handle_USER(&session, param);
    } else if (strcasecmp(command, "PASS") == 0) {
      handle_PASS(&session, param);
    } else if (strcasecmp(command, "SYST") == 0) {
      handle_SYST(&session);
    } else if (strcasecmp(command, "FEAT") == 0) {
      handle_FEAT(&session);
    } else if (strcasecmp(command, "PWD") == 0 ||
               strcasecmp(command, "XPWD") == 0) {
      handle_PWD(&session);
    } else if (strcasecmp(command, "CWD") == 0 ||
               strcasecmp(command, "XCWD") == 0) {
      handle_CWD(&session, param);
    } else if (strcasecmp(command, "CDUP") == 0 ||
               strcasecmp(command, "XCUP") == 0) {
      handle_CDUP(&session);
    } else if (strcasecmp(command, "TYPE") == 0) {
      handle_TYPE(&session, param);
    } else if (strcasecmp(command, "PASV") == 0) {
      handle_PASV(&session, g_server_ip);
    } else if (strcasecmp(command, "LIST") == 0 ||
               strcasecmp(command, "NLST") == 0) {
      handle_LIST(&session);
    } else if (strcasecmp(command, "RETR") == 0) {
      handle_RETR(&session, param);
    } else if (strcasecmp(command, "STOR") == 0) {
      handle_STOR(&session, param);
    } else if (strcasecmp(command, "QUIT") == 0) {
      handle_QUIT(&session);
      break;
    } else if (strcasecmp(command, "OPTS") == 0) {
      send_response(session.ctrl_socket, "200 OK\r\n");
    } else if (strcasecmp(command, "NOOP") == 0) {
      send_response(session.ctrl_socket, "200 NOOP OK\r\n");
    } else if (strcasecmp(command, "SIZE") == 0) {
      // Tra ve kich thuoc file
      char filepath[MAX_PATH * 2];
      get_absolute_path(&session, filepath);
      strcat(filepath, "/");
      strcat(filepath, param);

      struct stat st;
      if (stat(filepath, &st) == 0) {
        char response[64];
        sprintf(response, "213 %ld\r\n", (long)st.st_size);
        send_response(session.ctrl_socket, response);
      } else {
        send_response(session.ctrl_socket, "550 File not found\r\n");
      }
    } else {
      // Lenh khong ho tro
      send_response(session.ctrl_socket, "502 Command not implemented\r\n");
    }
  }

  // Don dep
  if (session.data_socket > 0) {
    close(session.data_socket);
  }
  if (session.pasv_listen_socket > 0) {
    close(session.pasv_listen_socket);
  }
  close(session.ctrl_socket);

  return NULL;
}

int main(int argc, char *argv[]) {
  // Xu ly tham so dong lenh
  int port = FTP_PORT;

  if (argc >= 2) {
    port = atoi(argv[1]);
  }
  if (argc >= 3) {
    strncpy(g_server_ip, argv[2], sizeof(g_server_ip) - 1);
  }

  printf("=== FTP Server ===\n");
  printf("Port: %d\n", port);
  printf("Server IP (cho PASV): %s\n", g_server_ip);
  printf("Nhan Ctrl+C de tat server\n\n");

  // Dang ky signal handler
  signal(SIGINT, signal_handler);

  // Doc danh sach tai khoan
  if (load_accounts(ACCOUNTS_FILE) < 0) {
    printf("Khong doc duoc file tai khoan, tao tai khoan mac dinh...\n");
    add_account("admin", "admin", "/tmp");
    add_account("user", "user", "/tmp");
    save_accounts(ACCOUNTS_FILE);
  }

  // Tao socket server
  g_server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (g_server_socket < 0) {
    printf("Khong the tao socket!\n");
    return 1;
  }

  // Cho phep reuse address
  int reuse = 1;
  setsockopt(g_server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  // Bind socket
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(g_server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    printf("Khong the bind port %d!\n", port);
    close(g_server_socket);
    return 1;
  }

  // Lang nghe ket noi
  listen(g_server_socket, 10);
  printf("Server dang lang nghe tren port %d...\n\n", port);

  // Vong lap chinh
  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_socket =
        accept(g_server_socket, (struct sockaddr *)&client_addr, &client_len);

    if (client_socket < 0) {
      continue;
    }

    printf("Client moi ket noi: %s:%d\n", inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));

    // Tao thread moi cho client
    pthread_t tid;
    int *arg = malloc(sizeof(int));
    *arg = client_socket;

    if (pthread_create(&tid, NULL, client_thread, arg) != 0) {
      printf("Khong the tao thread!\n");
      close(client_socket);
      free(arg);
    } else {
      pthread_detach(tid); // Khong can join thread
    }
  }

  close(g_server_socket);
  return 0;
}