// USER

if (strncmp(buffer, "USER ", 5) == 0) {
  char *username = buffer + 5;
  while (username[strlen(username) - 1] == '\r' ||
         username[strlen(username) - 1] == '\n') {
    username[strlen(username) - 1] = 0;
  }

  // Lưu username vào biến phiên (session) nếu cần
  printf("User login attempt: %s\n", username);

  const char *resp331 = "331 User name okay, need password.\r\n";
  Send(c, (char *)resp331, strlen(resp331));
}

// PASS
if (strncmp(buffer, "PASS ", 5) == 0) {
  char *password = buffer + 5;
  while (password[strlen(password) - 1] == '\r' ||
         password[strlen(password) - 1] == '\n') {
    password[strlen(password) - 1] = 0;
  }

  if (strcmp(password, "123456") == 0) {
    const char *resp230 = "230 User logged in, proceed.\r\n";
    Send(c, (char *)resp230, strlen(resp230));
  } else {
    const char *resp530 = "530 Not logged in.\r\n";
    Send(c, (char *)resp530, strlen(resp530));
  }
}

// PWD
if (strncmp(buffer, "PWD", 3) == 0) {
  char resp257[1024];
  // Giả sử 'currentRelativePath' lưu đường dẫn hiện tại (vd: "/")
  sprintf(resp257, "257 \"%s\" is current directory.\r\n", currentRelativePath);
  Send(c, resp257, strlen(resp257));
}

// CWD
if (strncmp(buffer, "CWD ", 4) == 0) {
  char *path = buffer + 4;
  while (path[strlen(path) - 1] == '\r' || path[strlen(path) - 1] == '\n') {
    path[strlen(path) - 1] = 0;
  }

  // Logic kiểm tra thư mục tồn tại và cập nhật currentRelativePath ở đây
  // Giả định thành công:
  const char *resp250 = "250 Directory successfully changed.\r\n";
  Send(c, (char *)resp250, strlen(resp250));
}

// STOR
if (strncmp(buffer, "STOR ", 5) == 0) {
  char *filename = buffer + 5;
  while (filename[strlen(filename) - 1] == '\r' ||
         filename[strlen(filename) - 1] == '\n') {
    filename[strlen(filename) - 1] = 0;
  }

  char absPath[1024] = {0};
  sprintf(absPath, "%s/%s", rootPath, filename);

  const char *resp150 = "150 Ok to send data.\r\n";
  Send(c, (char *)resp150, strlen(resp150));

  FILE *f = fopen(absPath, "wb"); // Mở để ghi nhị phân
  if (f != NULL) {
    char block[1024];
    int tmp;
    // Nhận dữ liệu từ dataSocket cho đến khi đóng kết nối
    while ((tmp = recv(dataSocket, block, sizeof(block), 0)) > 0) {
      fwrite(block, 1, tmp, f);
    }
    fclose(f);
  }

  close(dataSocket);
  const char *resp226 = "226 Closing data connection. File received.\r\n";
  Send(c, (char *)resp226, strlen(resp226));
}

// RETR
if (strncmp(buffer, "RETR ", 5) == 0) {
  char *filename = buffer + 5;
  while (filename[strlen(filename) - 1] == '\r' ||
         filename[strlen(filename) - 1] == '\n') {
    filename[strlen(filename) - 1] = 0;
  }

  char absPath[1024] = {0};
  if (rootPath[strlen(rootPath) - 1] == '/') {
    sprintf(absPath, "%s%s", rootPath, filename);
  } else {
    sprintf(absPath, "%s/%s", rootPath, filename);
  }

  const char *resp150 = "150 Start transferring\r\n";
  Send(c, (char *)resp150, strlen(resp150));

  FILE *f = fopen(absPath, "rb");
  if (f != NULL) {
    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fseek(f, 0, SEEK_SET);

    int r = 0;
    while (r < size) {
      char block[1024] = {0};
      int tmp = fread(block, 1, sizeof(block), f);
      if (tmp > 0) {
        r += tmp;
        if (Send(dataSocket, block, tmp) <= 0) {
          break;
        }
      } else {
        break;
      }
    }
    fclose(f);
  }

  close(dataSocket);
  const char *resp226 = "226 Transferring completed\r\n";
  Send(c, (char *)resp226, strlen(resp226));
}

// TYPE
if (strncmp(buffer, "TYPE ", 5) == 0) {
  // Thông thường đơn giản là phản hồi OK vì hầu hết client hiện đại dùng Binary
  const char *resp200 = "200 Switching to Binary mode.\r\n";
  Send(c, (char *)resp200, strlen(resp200));
}

// LIST
if (strncmp(buffer, "LIST", 4) == 0) {
  const char *resp150 = "150 Here comes the directory listing.\r\n";
  Send(c, (char *)resp150, strlen(resp150));

  DIR *d = opendir(rootPath);
  struct dirent *dir;
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      char line[1024];
      sprintf(line, "%s\r\n", dir->d_name);
      Send(dataSocket, line, strlen(line)); // Gửi qua kênh dữ liệu
    }
    closedir(d);
  }

  close(dataSocket);
  const char *resp226 = "226 Directory send OK.\r\n";
  Send(c, (char *)resp226, strlen(resp226));
}

// RNFR va RNTO
char renameFrom[1024]; // Biến lưu tạm

// Bước 1: RNFR
if (strncmp(buffer, "RNFR ", 5) == 0) {
  sscanf(buffer + 5, "%s", renameFrom);
  const char *resp350 = "350 File exists, ready for destination name.\r\n";
  Send(c, (char *)resp350, strlen(resp350));
}

// Bước 2: RNTO
if (strncmp(buffer, "RNTO ", 5) == 0) {
  char renameTo[1024];
  sscanf(buffer + 5, "%s", renameTo);

  char oldPath[1024], newPath[1024];
  sprintf(oldPath, "%s/%s", rootPath, renameFrom);
  sprintf(newPath, "%s/%s", rootPath, renameTo);

  if (rename(oldPath, newPath) == 0) {
    const char *resp250 = "250 Rename successful.\r\n";
    Send(c, (char *)resp250, strlen(resp250));
  } else {
    const char *resp550 = "550 Rename failed.\r\n";
    Send(c, (char *)resp550, strlen(resp550));
  }
}

// DELE - Xóa tệp
if (strncmp(buffer, "DELE ", 5) == 0) {
  char filename[1024];
  sscanf(buffer + 5, "%s", filename);
  char path[1024];
  sprintf(path, "%s/%s", rootPath, filename);

  if (remove(path) == 0) {
    Send(c, "250 Deleted.\r\n", 13);
  } else {
    Send(c, "550 Delete failed.\r\n", 20);
  }
}

// MKD - Tạo thư mục
if (strncmp(buffer, "MKD ", 4) == 0) {
  char foldername[1024];
  sscanf(buffer + 4, "%s", foldername);
  char path[1024];
  sprintf(path, "%s/%s", rootPath, foldername);

  if (mkdir(path, 0777) == 0) {
    Send(c, "257 Created.\r\n", 14);
  } else {
    Send(c, "550 Create failed.\r\n", 20);
  }
}

if (strncmp(buffer, "QUIT", 4) == 0) {
  const char *resp221 = "221 Goodbye.\r\n";
  Send(c, (char *)resp221, strlen(resp221));
  close(c); // Đóng kết nối điều khiển
  // Cần có logic break vòng lặp chính của thread xử lý client này
}

// PASV
if (strncmp(buffer, "PASV", 4) == 0) {
  // 1. Tạo socket lắng nghe cho kênh dữ liệu
  int pasv_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = 0; // Cổng 0 để hệ điều hành tự cấp cổng trống

  // 2. Bind và Listen
  bind(pasv_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  listen(pasv_fd, 1);

  // 3. Lấy thông tin cổng mà hệ điều hành đã cấp phát
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  getsockname(pasv_fd, (struct sockaddr *)&addr, &len);

  int port = ntohs(addr.sin_port);
  int p1 = port / 256;
  int p2 = port % 256;

  // 4. Lấy địa chỉ IP của Server (Giả sử là 127.0.0.1 để test local)
  // Trong thực tế bạn nên lấy IP thực của card mạng
  int h1 = 127, h2 = 0, h3 = 0, h4 = 1;

  // 5. Gửi phản hồi 227 cho Client
  char resp[1024];
  sprintf(resp, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).\r\n", h1, h2,
          h3, h4, p1, p2);
  Send(c, resp, strlen(resp));

  // 6. Chấp nhận kết nối từ Client (Accept)
  // Lưu ý: dataSocket sẽ được dùng cho các lệnh LIST, RETR, STOR sau đó
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  dataSocket = accept(pasv_fd, (struct sockaddr *)&client_addr, &client_len);

  // Sau khi accept thành công, ta có thể đóng socket lắng nghe tạm thời
  close(pasv_fd);
}
