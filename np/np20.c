/*
 * CHUONG TRINH: HTTP Upload Server - Server ho tro upload file qua HTTP
 *
 * KIEN THUC LAP TRINH MANG:
 * - HTTP protocol (RFC 2616): Request/Response format
 * - HTTP methods: GET (lay file/thu muc), POST (upload file)
 * - Content-Length header: Kich thuoc body tinh bang bytes
 * - multipart/form-data: Dinh dang upload file qua HTML form
 * - HTTP status codes: 200 OK, 404 Not Found, 500 Internal Error
 */

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <malloc.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

void Append(char **destination, const char *str) {
  char *tmp = *destination;
  int oldlen = tmp != NULL ? strlen(tmp) : 0;
  int more = strlen(str) + 1;

  // realloc(): Thay doi kich thuoc vung nho da cap phat
  // Giu nguyen du lieu cu, them vung nho moi
  tmp = (char *)realloc(tmp, oldlen + more);
  memset(tmp + oldlen, 0, more);
  sprintf(tmp + strlen(tmp), "%s", str);
  *destination = tmp;
}

int Compare(const struct dirent **e1, const struct dirent **e2) {
  // So sanh de sap xep: Thu muc truoc, file sau
  if ((*e1)->d_type == (*e2)->d_type) {
    return strcmp((*e1)->d_name, (*e2)->d_name);
  } else {
    if ((*e1)->d_type == DT_DIR)
      return -1;
    else
      return 1;
  }
}

void CreateHTML(const char *path, char **output) {
  struct dirent **result = NULL;

  // Tao HTML form cho upload file
  // enctype="multipart/form-data": Bat buoc khi upload file
  Append(output, "<html>");
  Append(output, "<form method=\"post\" enctype=\"multipart/form-data\">");
  Append(output, "<label>File to upload: </label>");
  Append(output, "<input type=\"file\" id=\"file1\" name=\"file1\"/> <br>");
  Append(output, "<input type=\"submit\" value=\"Upload\"><br>");
  Append(output, "</form>");

  int n = scandir(path, &result, NULL, Compare);
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      if (result[i]->d_type == DT_DIR) {
        Append(output, "<a href=\"");
        Append(output, path);
        if (path[strlen(path) - 1] != '/') {
          Append(output, "/");
        }
        Append(output, result[i]->d_name);
        Append(output, "\">");
        Append(output, "<b>");
        Append(output, result[i]->d_name);
        Append(output, "</b>");
        Append(output, "</a>");
        Append(output, "<br>");
      } else {
        // Them "?" cuoi URL de danh dau la file (khong phai thu muc)
        Append(output, "<a href=\"");
        Append(output, path);
        if (path[strlen(path) - 1] != '/') {
          Append(output, "/");
        }
        Append(output, result[i]->d_name);
        Append(output, "?");
        Append(output, "\">");
        Append(output, "<i>");
        Append(output, result[i]->d_name);
        Append(output, "</i>");
        Append(output, "</a>");
        Append(output, "<br>");
      }
      free(result[i]);
      result[i] = NULL;
    }
  }

  Append(output, "</html>");
}

// Ham Send dam bao gui HET du lieu
// send() co the khong gui het trong 1 lan (nhat la data lon)
void Send(int c, char *data, int len) {
  int sent = 0;
  // Vong lap gui cho den khi HET data hoac loi
  while (sent < len) {
    // Gui phan con lai: data + sent = vi tri bat dau, len - sent = so bytes con
    int tmp = send(c, data + sent, len - sent, 0);
    if (tmp > 0) {
      sent += tmp;
    } else
      break; // Loi hoac connection dong
  }
}

void *ClientThread(void *arg) {
  int c = *((int *)arg);
  free(arg);
  arg = NULL;

  char *requestBody = NULL;
  char *partBody = NULL;
  int bodyLen = 0;
  int partLen = 0;

  // Doc HTTP request header cho den khi gap "\r\n\r\n" (end of headers)
  // HTTP protocol dung "\r\n\r\n" de ngan cach header va body
  while (requestBody == NULL || strstr(requestBody, "\r\n\r\n") == NULL) {
    char chr;
    // Doc tung byte de tim header boundary
    // Cach nay cham nhung don gian, de hieu
    int r = recv(c, &chr, 1, 0);
    if (r > 0) {
      // Dynamic buffer: Tang kich thuoc khi can
      requestBody = (char *)realloc(requestBody, bodyLen + 1);
      requestBody[bodyLen] = chr;
      bodyLen++;
    } else
      break;
  }

  if (requestBody != NULL && strstr(requestBody, "\r\n\r\n") != NULL) {
    int contentRead = 0;
    int contentLength = 0;

    // Parse Content-Length header
    // strstr() tim vi tri "Content-Length: " trong request
    char *ctLen = strstr(requestBody, "Content-Length: ");
    if (ctLen != NULL) {
      // sscanf doc so sau "Content-Length: "
      sscanf(ctLen + strlen("Content-Length: "), "%d", &contentLength);
    }

    printf("%s", requestBody);

    // Parse HTTP method va path tu request line
    // Format: "METHOD /path HTTP/1.1\r\n..."
    char method[8] = {0};
    char path[1024] = {0};
    sscanf(requestBody, "%s%s", method, path);

    // Decode URL: Thay %20 bang space
    // URL encoding: Dau cach -> %20
    char tmp[1024] = {0};
    while (strstr(path, "%20") != NULL) {
      memset(tmp, 0, sizeof(tmp));
      char *space = strstr(path, "%20");
      // strncpy: Copy n bytes (phan truoc %20)
      strncpy(tmp, path, space - path);
      sprintf(tmp + strlen(tmp), " %s", space + 3);
      memcpy(path, tmp, sizeof(tmp));
    }

    // strcmp(): So sanh 2 chuoi
    // Tra ve 0 neu BANG, khac 0 neu khac
    if (strcmp(method, "GET") == 0) {
      // Xu ly favicon.ico (icon trang web)
      if (strstr(path, "favicon.ico") != NULL) {
        FILE *f = fopen("favicon.ico", "rb");
        if (f != NULL) {
          // Lay kich thuoc file
          fseek(f, 0, SEEK_END);
          int length = ftell(f);
          fseek(f, 0, SEEK_SET);

          char *icoData = (char *)calloc(length, 1);
          fread(icoData, 1, length, f);
          fclose(f);

          // HTTP Response format:
          // Status-Line: "HTTP/1.1 200 OK\r\n"
          // Headers: "Content-Length: xxx\r\n..."
          // CRLF: "\r\n"
          // Body: file data
          char ok[1024] = {0};
          sprintf(ok,
                  "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: "
                  "image/x-icon\r\n\r\n",
                  length);
          Send(c, ok, strlen(ok));
          Send(c, icoData, length);
        } else {
          char *notFound = "HTTP/1.1 404 NOT FOUND\r\n\r\n";
          Send(c, notFound, strlen(notFound));
        }
      } else {
        // Neu path ket thuc bang "?" -> Download file
        // Neu khong -> Hien thi thu muc
        if (path[strlen(path) - 1] != '?') {
          char *html = NULL;
          CreateHTML(path, &html);
          char ok[1024] = {0};
          sprintf(ok,
                  "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: "
                  "text/html\r\n\r\n",
                  (int)strlen(html));
          Send(c, ok, strlen(ok));
          Send(c, html, strlen(html));
        } else {
          // Download file
          path[strlen(path) - 1] = 0; // Bo dau "?"
          char mime_type[1024] = {0};

          // Xac dinh MIME type dua tren extension
          // MIME type cho browser biet loai file de xu ly dung
          if (strstr(path, ".c") != NULL) {
            sprintf(mime_type, "text/plain");
          } else if (strstr(path, ".jpg") != NULL) {
            sprintf(mime_type, "image/jpeg");
          } else if (strstr(path, ".png") != NULL) {
            sprintf(mime_type, "image/png");
          } else if (strstr(path, ".pdf") != NULL) {
            sprintf(mime_type, "application/pdf");
          } else if (strstr(path, ".mp4") != NULL) {
            sprintf(mime_type, "video/mp4");
          } else {
            // application/octet-stream: Binary data (download)
            sprintf(mime_type, "application/octet-stream");
          }

          char response[1024] = {0};
          FILE *f = fopen(path, "rb");
          if (f != NULL) {
            fseek(f, 0, SEEK_END);
            int len = ftell(f);
            fseek(f, 0, SEEK_SET);
            sprintf(response,
                    "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: "
                    "%s\r\n\r\n",
                    len, mime_type);
            Send(c, response, strlen(response));

            // Gui file theo tung block 1MB
            // Tranh doc toan bo file lon vao RAM
            int block_size = 1024 * 1024;
            char *data = (char *)calloc(block_size, 1);
            int read = 0;
            while (read < len) {
              int tmpr = fread(data, 1, block_size, f);
              Send(c, data, tmpr);
              read += tmpr;
            }
            free(data);
            data = NULL;
          } else {
            sprintf(response, "HTTP/1.1 404 NOT FOUND\r\n\r\n");
            Send(c, response, strlen(response));
          }
          fclose(f);
        }
      }
    } else if (strcmp(method, "POST") == 0) {
      // Xu ly file upload (multipart/form-data)
      // Doc part header cho den "\r\n\r\n"
      while (partBody == NULL || strstr(partBody, "\r\n\r\n") == NULL) {
        char chr;
        int r = recv(c, &chr, 1, 0);
        if (r > 0) {
          partBody = (char *)realloc(partBody, partLen + 1);
          partBody[partLen] = chr;
          partLen++;
          contentRead++;
        } else
          break;
      }

      if (strstr(partBody, "\r\n\r\n") != NULL) {
        // Parse filename tu Content-Disposition header
        // Format: Content-Disposition: form-data; name="file1";
        // filename="abc.txt"
        char *filename = strstr(partBody, "filename=");
        if (filename != NULL) {
          char name[1024] = {0};
          sscanf(filename + 10, "%s", name); // Skip 'filename="'
          name[strlen(name) - 1] = 0;        // Bo dau '"' cuoi

          char fullpath[2048] = {0};
          if (path[strlen(path) - 1] == '/')
            sprintf(fullpath, "%s%s", path, name);
          else
            sprintf(fullpath, "%s/%s", path, name);

          FILE *f = fopen(fullpath, "wb");
          if (f != NULL) {
            // Doc va ghi file data
            while (contentRead < contentLength) {
              char data[1024] = {0};
              int r = recv(c, data, sizeof(data), 0);
              if (r > 0) {
                fwrite(data, 1, r, f);
                contentRead += r;
              } else
                break;
            }
            fclose(f);
            char *response = "HTTP/1.1 200 OK\r\n\r\n";
            Send(c, response, strlen(response));
          } else {
            char *response = "HTTP/1.1 500 INTERNAL SERVER ERROR\r\n\r\n";
            Send(c, response, strlen(response));
          }
        } else {
          char *response = "HTTP/1.1 400 BAD REQUEST\r\n\r\n";
          Send(c, response, strlen(response));
        }
      } else {
        char *response = "HTTP/1.1 400 BAD REQUEST\r\n\r\n";
        Send(c, response, strlen(response));
      }
    }
  }

  close(c);
  return NULL;
}

int main() {
  SOCKADDR_IN saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = inet_addr("0.0.0.0");
  saddr.sin_port = htons(8888);

  int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (bind(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR)) == 0) {
    listen(s, 10);
    while (0 == 0) {
      int c = accept(s, NULL, 0);
      pthread_t tid = 0;
      int *arg = (int *)calloc(1, sizeof(int));
      *arg = c;
      pthread_create(&tid, NULL, ClientThread, arg);
    }
  } else {
    printf("Failed to bind\n");
  }
}