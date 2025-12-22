/*
 * ftp_client.h - Xu ly cac lenh FTP cho client
 *
 * Chua cac ham gui lenh va nhan phan hoi tu server
 */

#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H

#define BUFFER_SIZE 4096
#define MAX_PATH 512

// Struct luu thong tin phien FTP
typedef struct {
  int ctrl_socket;            // Socket dieu khien
  int data_socket;            // Socket du lieu (PASV)
  char server_ip[32];         // IP server
  int server_port;            // Port server
  char current_dir[MAX_PATH]; // Thu muc hien tai
  int logged_in;              // Da dang nhap chua
} FTPSession;

// Ket noi den FTP server
// Tra ve 0 neu thanh cong, -1 neu loi
int ftp_connect(FTPSession *session, const char *host, int port);

// Ngat ket noi
void ftp_disconnect(FTPSession *session);

// Gui lenh den server (tu dong them \r\n)
// Tra ve so byte gui duoc
int ftp_send_command(FTPSession *session, const char *cmd);

// Nhan phan hoi tu server
// Tra ve response code (vd: 220, 331, 230...)
int ftp_recv_response(FTPSession *session, char *buffer, int size);

// Gui lenh va doi phan hoi
// Tra ve response code
int ftp_execute(FTPSession *session, const char *cmd, char *response, int size);

// Dang nhap
int ftp_login(FTPSession *session, const char *username, const char *password);

// Lay thu muc hien tai
int ftp_pwd(FTPSession *session, char *path);

// Doi thu muc
int ftp_cwd(FTPSession *session, const char *path);

// Thiet lap PASV mode va lay thong tin data port
// Tra ve data socket, -1 neu loi
int ftp_pasv(FTPSession *session);

// Liet ke thu muc
int ftp_list(FTPSession *session);

// Download file
int ftp_download(FTPSession *session, const char *remote_file,
                 const char *local_file);

// Upload file
int ftp_upload(FTPSession *session, const char *local_file,
               const char *remote_file);

// Ngat ket noi
int ftp_quit(FTPSession *session);

#endif // FTP_CLIENT_H
