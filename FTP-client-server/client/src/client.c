/*
 * client.c - FTP Client chinh
 *
 * KIEN THUC LAP TRINH MANG:
 * - FTP client: Gui lenh va nhan phan hoi tu server
 * - Interactive shell: Nhap lenh tu ban phim
 * - Log theo dinh dang: hh:mm:ss <Lenh gui di> <Phan hoi server>
 *
 * KIEN THUC C ADVANCE:
 * - Command parsing: Tach lenh va tham so
 * - String handling: Xu ly chuoi nguoi dung nhap
 */

#include "ftp_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Lay thoi gian hien tai dang hh:mm:ss
void get_time_str(char *buffer, int size) {
  time_t now = time(NULL);
  struct tm *tm = localtime(&now);
  strftime(buffer, size, "%H:%M:%S", tm);
}

// Log lenh va phan hoi
void log_message(const char *cmd, const char *response) {
  char time_str[16];
  get_time_str(time_str, sizeof(time_str));

  // Chi lay dong dau cua response
  char resp_line[256];
  strncpy(resp_line, response, sizeof(resp_line) - 1);
  char *newline = strchr(resp_line, '\n');
  if (newline)
    *newline = '\0';

  printf("%s %s %s\n", time_str, cmd, resp_line);
}

// In huong dan su dung
void print_help() {
  printf("\n=== Cac lenh FTP ===\n");
  printf("  pwd          - Hien thi thu muc hien tai\n");
  printf("  ls / dir     - Liet ke file trong thu muc\n");
  printf("  cd <path>    - Doi thu muc (vd: cd /home)\n");
  printf("  get <file>   - Download file (vd: get note.txt)\n");
  printf("  put <file>   - Upload file (vd: put test.txt)\n");
  printf("  quit / exit  - Thoat\n");
  printf("  help         - Hien thi huong dan nay\n\n");
}

int main(int argc, char *argv[]) {
  // Kiem tra tham so
  if (argc < 3) {
    printf("Su dung: %s <host> <port> [username] [password]\n", argv[0]);
    printf("Vi du: %s localhost 2121 admin admin\n", argv[0]);
    return 1;
  }

  char *host = argv[1];
  int port = atoi(argv[2]);
  char *username = (argc >= 4) ? argv[3] : NULL;
  char *password = (argc >= 5) ? argv[4] : NULL;

  printf("=== FTP Client ===\n");
  printf("Dang ket noi den %s:%d...\n", host, port);

  // Ket noi den server
  FTPSession session;
  if (ftp_connect(&session, host, port) != 0) {
    printf("Khong the ket noi den server!\n");
    return 1;
  }

  printf("Da ket noi thanh cong!\n");

  // Dang nhap
  if (username == NULL) {
    // Nhap username tu ban phim
    char user_input[64];
    printf("Username: ");
    fgets(user_input, sizeof(user_input), stdin);
    user_input[strcspn(user_input, "\r\n")] = '\0';
    username = user_input;

    char pass_input[64];
    printf("Password: ");
    fgets(pass_input, sizeof(pass_input), stdin);
    pass_input[strcspn(pass_input, "\r\n")] = '\0';
    password = pass_input;

    if (ftp_login(&session, username, password) != 0) {
      printf("Dang nhap that bai!\n");
      ftp_disconnect(&session);
      return 1;
    }
  } else {
    if (ftp_login(&session, username, password) != 0) {
      printf("Dang nhap that bai!\n");
      ftp_disconnect(&session);
      return 1;
    }
  }

  printf("Dang nhap thanh cong!\n");
  print_help();

  // Vong lap xu ly lenh
  char line[512];
  char response[BUFFER_SIZE];

  while (1) {
    printf("ftp> ");
    fflush(stdout);

    if (fgets(line, sizeof(line), stdin) == NULL) {
      break;
    }

    // Bo ky tu xuong dong
    line[strcspn(line, "\r\n")] = '\0';

    // Bo qua lenh trong
    if (strlen(line) == 0) {
      continue;
    }

    // Parse lenh va tham so
    char cmd[64] = {0};
    char param[256] = {0};
    sscanf(line, "%s %[^\n]", cmd, param);

    char time_str[16];
    get_time_str(time_str, sizeof(time_str));

    // Xu ly cac lenh
    if (strcasecmp(cmd, "quit") == 0 || strcasecmp(cmd, "exit") == 0) {
      printf("%s QUIT Goodbye\n", time_str);
      ftp_quit(&session);
      break;
    } else if (strcasecmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
      print_help();
    } else if (strcasecmp(cmd, "pwd") == 0) {
      char path[MAX_PATH];
      if (ftp_pwd(&session, path) == 0) {
        printf("%s PWD Thu muc hien tai: %s\n", time_str, path);
      } else {
        printf("%s PWD Loi khi lay thu muc\n", time_str);
      }
    } else if (strcasecmp(cmd, "ls") == 0 || strcasecmp(cmd, "dir") == 0 ||
               strcasecmp(cmd, "list") == 0) {
      printf("%s LIST\n", time_str);
      ftp_list(&session);
    } else if (strcasecmp(cmd, "cd") == 0) {
      if (strlen(param) == 0) {
        printf("Su dung: cd <duong_dan>\n");
      } else {
        if (ftp_cwd(&session, param) == 0) {
          printf("%s CWD Da chuyen sang: %s\n", time_str, param);
        } else {
          printf("%s CWD Khong the chuyen thu muc\n", time_str);
        }
      }
    } else if (strcasecmp(cmd, "get") == 0 ||
               strcasecmp(cmd, "download") == 0 ||
               strcasecmp(cmd, "retr") == 0) {
      if (strlen(param) == 0) {
        printf("Su dung: get <ten_file>\n");
      } else {
        // Lay ten file local (neu khong chi dinh, dung ten remote)
        char *remote_file = param;
        char *local_file = param;
        char *space = strchr(param, ' ');
        if (space) {
          *space = '\0';
          local_file = space + 1;
        }

        printf("%s RETR Dang download %s...\n", time_str, remote_file);
        if (ftp_download(&session, remote_file, local_file) == 0) {
          printf("%s RETR Download thanh cong\n", time_str);
        } else {
          printf("%s RETR Download that bai\n", time_str);
        }
      }
    } else if (strcasecmp(cmd, "put") == 0 || strcasecmp(cmd, "upload") == 0 ||
               strcasecmp(cmd, "stor") == 0) {
      if (strlen(param) == 0) {
        printf("Su dung: put <ten_file_local> [ten_file_remote]\n");
      } else {
        // Lay ten file remote (neu khong chi dinh, dung ten local)
        char *local_file = param;
        char *remote_file = param;
        char *space = strchr(param, ' ');
        if (space) {
          *space = '\0';
          remote_file = space + 1;
        }

        printf("%s STOR Dang upload %s...\n", time_str, local_file);
        if (ftp_upload(&session, local_file, remote_file) == 0) {
          printf("%s STOR Upload thanh cong\n", time_str);
        } else {
          printf("%s STOR Upload that bai\n", time_str);
        }
      }
    } else {
      printf("Lenh khong hop le. Nhap 'help' de xem huong dan.\n");
    }
  }

  printf("Tam biet!\n");
  return 0;
}