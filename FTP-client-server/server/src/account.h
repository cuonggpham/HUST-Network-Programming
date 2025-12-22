/*
 * account.h - Quan ly tai khoan FTP
 *
 * Chua cac ham va struct de quan ly danh sach tai khoan
 * Moi tai khoan gom: username, password, thu muc goc
 */

#ifndef ACCOUNT_H
#define ACCOUNT_H

#define MAX_ACCOUNTS 100
#define MAX_USERNAME 64
#define MAX_PASSWORD 64
#define MAX_PATH_LEN 256

// Struct luu thong tin 1 tai khoan
typedef struct {
  char username[MAX_USERNAME];
  char password[MAX_PASSWORD];
  char root_dir[MAX_PATH_LEN]; // Thu muc goc cua user
} Account;

// Danh sach tai khoan toan cuc
extern Account g_accounts[MAX_ACCOUNTS];
extern int g_account_count;

// Doc danh sach tai khoan tu file
// Tra ve so luong tai khoan doc duoc
int load_accounts(const char *filename);

// Ghi danh sach tai khoan ra file
// Tra ve 0 neu thanh cong, -1 neu loi
int save_accounts(const char *filename);

// Them tai khoan moi
// Tra ve 0 neu thanh cong, -1 neu loi (trung username hoac day)
int add_account(const char *username, const char *password,
                const char *root_dir);

// Kiem tra dang nhap
// Tra ve index cua account trong mang neu dung, -1 neu sai
int validate_login(const char *username, const char *password);

// Lay thu muc goc cua user
// Tra ve NULL neu khong tim thay
const char *get_user_root_dir(const char *username);

#endif // ACCOUNT_H
