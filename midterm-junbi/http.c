// X
if (strcmp(path, "/favicon.ico") == 0) {
  char response[1024] = {0};
  const char *html = "RESOURCE NOT FOUND";
  sprintf(response,
          "HTTP/1.1 404 NOT FOUND\r\nContent-Type: "
          "text/html\r\nContent-Length: %d\r\n\r\n",
          (int)strlen(html));

  Send(c, (char *)response, strlen(response));
  Send(c, (char *)html, strlen(html));
  close(c);
} else if (strstr(path, "*") != NULL) // Xử lý FOLDER
{
  path[strlen(path) - 1] = 0; // Bỏ dấu * ở cuối đường dẫn
  char *html = BuildHTML(path);
  char response[1024] = {0};

  sprintf(response,
          "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: "
          "%d\r\n\r\n",
          (int)strlen(html));

  Send(c, (char *)response, strlen(response));
  Send(c, (char *)html, strlen(html));

  // Lưu ý: Nếu BuildHTML dùng malloc, nên free(html) ở đây nếu cần thiết
  close(c);
}

// Xu ly file
FILE *f = fopen(path, "rb");
if (f != NULL) {
  // Lấy kích thước file
  fseek(f, 0, SEEK_END);
  int size = ftell(f);
  fseek(f, 0, SEEK_SET);

  // Cấp phát bộ nhớ và đọc dữ liệu
  char *data = (char *)calloc(size, 1);
  int r = 0;
  while (r < size) {
    int tmp = fread(data + r, 1, size - r, f);
    if (tmp <= 0)
      break;
    r += tmp;
  }

  // Xác định Content-Type dựa trên đuôi file
  char httpok[1024] = {0};
  char ct[128] = {0};
  if (strstr(path, ".txt") != NULL) {
    strcpy(ct, "text/plain"); // Thường .txt là text/plain, bạn có thể đổi thành
                              // text/html nếu muốn
  } else if (strstr(path, ".png") != NULL) {
    strcpy(ct, "image/png");
  } else if (strstr(path, ".mp4") != NULL) {
    strcpy(ct, "video/mp4");
  } else if (strstr(path, ".pdf") != NULL) {
    strcpy(ct, "application/pdf");
  } else {
    strcpy(ct, "application/octet-stream");
  }

  // Gửi header HTTP 200 OK
  sprintf(httpok,
          "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n",
          ct, size);
  Send(c, httpok, strlen(httpok));

  // Gửi nội dung file
  Send(c, data, size);

  // Giải phóng bộ nhớ và đóng kết nối
  free(data);
  fclose(f);
  close(c);
} else {
  // Xử lý lỗi 404 RESOURCE NOT FOUND
  char http404[1024] = {0};
  const char *html404 = "RESOURCE NOT FOUND";
  sprintf(http404,
          "HTTP/1.1 404 NOT FOUND\r\nContent-Type: "
          "text/html\r\nContent-Length: %d\r\n\r\n",
          (int)strlen(html404));

  Send(c, (char *)http404, strlen(http404));
  Send(c, (char *)html404, strlen(html404));
  close(c);
}

// Chuyen huong
else if (strcmp(path, "/old-page.html") == 0) {
  char response[1024] = {0};
  const char *new_location = "/new-page.html";

  // Header 301 yêu cầu thêm trường Location
  sprintf(response,
          "HTTP/1.1 301 Moved Permanently\r\n"
          "Location: %s\r\n"
          "Content-Length: 0\r\n"
          "Connection: close\r\n\r\n",
          new_location);

  Send(c, response, strlen(response));
  close(c);
}

// Tra ve JSON
else if (strcmp(path, "/api/status") == 0) {
  char response[1024] = {0};
  const char *json_data =
      "{\"status\": \"running\", \"uptime\": \"24h\", \"version\": \"1.0.0\"}";

  sprintf(response,
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: application/json\r\n"
          "Content-Length: %d\r\n"
          "Access-Control-Allow-Origin: *\r\n\r\n",
          (int)strlen(json_data));

  Send(c, response, strlen(response));
  Send(c, (char *)json_data, strlen(json_data));
  close(c);
}

// 405 Not Allowed
// Giả sử biến 'method' lưu phương thức HTTP (GET, POST,...)
if (strcmp(method, "POST") != 0 && strcmp(method, "GET") != 0) {
  char response[1024] = {0};
  const char *body = "<h1>405 Method Not Allowed</h1>";

  sprintf(response,
          "HTTP/1.1 405 Method Not Allowed\r\n"
          "Allow: GET, POST\r\n"
          "Content-Type: text/html\r\n"
          "Content-Length: %d\r\n\r\n",
          (int)strlen(body));

  Send(c, response, strlen(response));
  Send(c, (char *)body, strlen(body));
  close(c);
}

// 403 Forbidden
else if (strstr(path, ".c") != NULL || strstr(path, ".h") != NULL) {
  char response[1024] = {0};
  const char *msg = "ACCESS DENIED";

  sprintf(response,
          "HTTP/1.1 403 Forbidden\r\n"
          "Content-Type: text/plain\r\n"
          "Content-Length: %d\r\n\r\n",
          (int)strlen(msg));

  Send(c, response, strlen(response));
  Send(c, (char *)msg, strlen(msg));
  close(c);
}

// Plain text
else if (strcmp(path, "/robots.txt") == 0) {
  char response[1024] = {0};
  const char *content = "User-agent: *\nDisallow: /private/";

  sprintf(response,
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/plain\r\n"
          "Content-Length: %d\r\n\r\n",
          (int)strlen(content));

  Send(c, response, strlen(response));
  Send(c, (char *)content, strlen(content));
  close(c);
}
