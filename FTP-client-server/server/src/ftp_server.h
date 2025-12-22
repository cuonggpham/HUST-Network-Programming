/*
 * ftp_server.h - Xu ly cac lenh FTP
 *
 * Chua cac struct va ham de xu ly cac lenh FTP tu client
 * Bao gom: USER, PASS, PWD, CWD, LIST, PASV, RETR, STOR, QUIT
 */

#ifndef FTP_SERVER_H
#define FTP_SERVER_H

#include <netinet/in.h>

#define BUFFER_SIZE 4096
#define MAX_PATH 512

// Struct luu thong tin phien lam viec cua client
typedef struct {
  int ctrl_socket;            // Socket dieu khien (port 21)
  int data_socket;            // Socket truyen du lieu (PASV)
  int pasv_listen_socket;     // Socket lang nghe PASV
  int logged_in;              // Da dang nhap chua (0/1)
  char username[64];          // Ten dang nhap
  char root_dir[MAX_PATH];    // Thu muc goc cua user
  char current_dir[MAX_PATH]; // Thu muc hien tai (tuong doi voi root)
  char client_ip[32];         // IP cua client
  int pasv_port;              // Port cho PASV mode
} ClientSession;

// Gui response den client qua control connection
void send_response(int socket, const char *response);

// Xu ly lenh USER
void handle_USER(ClientSession *session, const char *username);

// Xu ly lenh PASS
void handle_PASS(ClientSession *session, const char *password);

// Xu ly lenh SYST
void handle_SYST(ClientSession *session);

// Xu ly lenh FEAT
void handle_FEAT(ClientSession *session);

// Xu ly lenh PWD
void handle_PWD(ClientSession *session);

// Xu ly lenh CWD
void handle_CWD(ClientSession *session, const char *path);

// Xu ly lenh CDUP
void handle_CDUP(ClientSession *session);

// Xu ly lenh TYPE
void handle_TYPE(ClientSession *session, const char *type);

// Xu ly lenh PASV
void handle_PASV(ClientSession *session, const char *server_ip);

// Xu ly lenh LIST
void handle_LIST(ClientSession *session);

// Xu ly lenh RETR (download)
void handle_RETR(ClientSession *session, const char *filename);

// Xu ly lenh STOR (upload)
void handle_STOR(ClientSession *session, const char *filename);

// Xu ly lenh QUIT
void handle_QUIT(ClientSession *session);

// Ham ho tro: Lay duong dan tuyet doi tu current_dir
void get_absolute_path(ClientSession *session, char *abs_path);

#endif // FTP_SERVER_H
