/*
 * account.c - Quan ly tai khoan FTP
 *
 * Implement cac ham doc/ghi file tai khoan
 * Format file: username:password:/path/to/root (moi dong 1 account)
 */

#include "account.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Danh sach tai khoan toan cuc
Account g_accounts[MAX_ACCOUNTS];
int g_account_count = 0;

// Doc danh sach tai khoan tu file
int load_accounts(const char *filename) {
  FILE *f = fopen(filename, "r");
  if (f == NULL) {
    printf("Khong the mo file tai khoan: %s\n", filename);
    return -1;
  }

  g_account_count = 0;
  char line[512];

  while (fgets(line, sizeof(line), f) != NULL &&
         g_account_count < MAX_ACCOUNTS) {
    // Bo ky tu xuong dong
    line[strcspn(line, "\r\n")] = 0;

    // Bo qua dong trong hoac dong comment
    if (line[0] == '\0' || line[0] == '#') {
      continue;
    }

    // Parse dong: username:password:root_dir
    char *token = strtok(line, ":");
    if (token == NULL)
      continue;
    strncpy(g_accounts[g_account_count].username, token, MAX_USERNAME - 1);

    token = strtok(NULL, ":");
    if (token == NULL)
      continue;
    strncpy(g_accounts[g_account_count].password, token, MAX_PASSWORD - 1);

    token = strtok(NULL, ":");
    if (token == NULL)
      continue;
    strncpy(g_accounts[g_account_count].root_dir, token, MAX_PATH_LEN - 1);

    g_account_count++;
  }

  fclose(f);
  printf("Da doc %d tai khoan tu file.\n", g_account_count);
  return g_account_count;
}

// Ghi danh sach tai khoan ra file
int save_accounts(const char *filename) {
  FILE *f = fopen(filename, "w");
  if (f == NULL) {
    printf("Khong the ghi file tai khoan: %s\n", filename);
    return -1;
  }

  fprintf(f, "# Danh sach tai khoan FTP\n");
  fprintf(f, "# Format: username:password:root_directory\n\n");

  for (int i = 0; i < g_account_count; i++) {
    fprintf(f, "%s:%s:%s\n", g_accounts[i].username, g_accounts[i].password,
            g_accounts[i].root_dir);
  }

  fclose(f);
  printf("Da luu %d tai khoan ra file.\n", g_account_count);
  return 0;
}

// Them tai khoan moi
int add_account(const char *username, const char *password,
                const char *root_dir) {
  // Kiem tra da day chua
  if (g_account_count >= MAX_ACCOUNTS) {
    printf("Danh sach tai khoan da day!\n");
    return -1;
  }

  // Kiem tra trung username
  for (int i = 0; i < g_account_count; i++) {
    if (strcmp(g_accounts[i].username, username) == 0) {
      printf("Username da ton tai: %s\n", username);
      return -1;
    }
  }

  // Them tai khoan moi
  strncpy(g_accounts[g_account_count].username, username, MAX_USERNAME - 1);
  strncpy(g_accounts[g_account_count].password, password, MAX_PASSWORD - 1);
  strncpy(g_accounts[g_account_count].root_dir, root_dir, MAX_PATH_LEN - 1);
  g_account_count++;

  printf("Da them tai khoan: %s\n", username);
  return 0;
}

// Kiem tra dang nhap
int validate_login(const char *username, const char *password) {
  for (int i = 0; i < g_account_count; i++) {
    if (strcmp(g_accounts[i].username, username) == 0 &&
        strcmp(g_accounts[i].password, password) == 0) {
      return i; // Tra ve index cua account
    }
  }
  return -1; // Sai thong tin dang nhap
}

// Lay thu muc goc cua user
const char *get_user_root_dir(const char *username) {
  for (int i = 0; i < g_account_count; i++) {
    if (strcmp(g_accounts[i].username, username) == 0) {
      return g_accounts[i].root_dir;
    }
  }
  return NULL;
}