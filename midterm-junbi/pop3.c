// STAT
if (strncmp(buffer, "STAT", 4) == 0) {
  int count = 0;
  long totalSize = 0;

  // Quét thư mục rootPath để đếm file và tính size
  DIR *d = opendir(rootPath);
  struct dirent *dir;
  while (d && (dir = readdir(d)) != NULL) {
    if (dir->d_type == DT_REG) { // Chỉ tính file thường
      count++;
      char fullPath[1024];
      sprintf(fullPath, "%s/%s", rootPath, dir->d_name);
      struct stat st;
      stat(fullPath, &st);
      totalSize += st.st_size;
    }
  }
  closedir(d);

  char resp[128];
  sprintf(resp, "+OK %d %ld\r\n", count, totalSize);
  Send(c, resp, strlen(resp));
}

// LIST
else if (strncmp(buffer, "LIST", 4) == 0) {
  Send(c, "+OK Mailbox scan listing follows\r\n", 34);

  DIR *d = opendir(rootPath);
  struct dirent *dir;
  int index = 1;
  while (d && (dir = readdir(d)) != NULL) {
    if (dir->d_type == DT_REG) {
      if (!deleted[index]) { // Chỉ hiện những mail chưa đánh dấu xóa
        char fullPath[1024], line[128];
        sprintf(fullPath, "%s/%s", rootPath, dir->d_name);
        struct stat st;
        stat(fullPath, &st);

        sprintf(line, "%d %ld\r\n", index, st.st_size);
        Send(c, line, strlen(line));
      }
      index++;
    }
  }
  closedir(d);
  Send(c, ".\r\n", 3); // Dấu chấm kết thúc danh sách trong POP3
}

// RETR
else if (strncmp(buffer, "RETR ", 5) == 0) {
  int msgNum = atoi(buffer + 5);
  if (deleted[msgNum]) {
    Send(c, "-ERR message already deleted\r\n", 30);
  } else {
    // Tìm file thứ msgNum trong thư mục (Logic tìm file tương tự LIST)
    char targetFile[1024] = "";
    /* ... Logic tìm tên file của email thứ msgNum ... */

    FILE *f = fopen(targetFile, "rb");
    if (f) {
      Send(c, "+OK message follows\r\n", 21);
      char block[1024];
      int n;
      while ((n = fread(block, 1, sizeof(block), f)) > 0) {
        Send(c, block, n);
      }
      fclose(f);
      Send(c, "\r\n.\r\n", 5); // Kết thúc email bằng \r\n.\r\n
    } else {
      Send(c, "-ERR no such message\r\n", 22);
    }
  }
}

// DELE
else if (strncmp(buffer, "DELE ", 5) == 0) {
  int msgNum = atoi(buffer + 5);
  // Kiểm tra xem msgNum có hợp lệ không...
  deleted[msgNum] = 1;
  Send(c, "+OK message marked for deletion\r\n", 33);
}

// NOOP
else if (strncmp(buffer, "DELE ", 5) == 0) {
  int msgNum = atoi(buffer + 5);
  // Kiểm tra xem msgNum có hợp lệ không...
  deleted[msgNum] = 1;
  Send(c, "+OK message marked for deletion\r\n", 33);
}

// RSET
else if (strncmp(buffer, "RSET", 4) == 0) {
  for (int i = 0; i < 101; i++)
    deleted[i] = 0;
  Send(c, "+OK maildrop has all messages restored\r\n", 40);
}
