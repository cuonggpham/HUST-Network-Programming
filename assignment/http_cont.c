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
  tmp = (char *)realloc(tmp, oldlen + more);
  memset(tmp + oldlen, 0, more);
  sprintf(tmp + strlen(tmp), "%s", str);
  *destination = tmp;
}

int Compare(const struct dirent **e1, const struct dirent **e2) {
  if ((*e1)->d_type == (*e2)->d_type) {
    return strcmp((*e1)->d_name, (*e2)->d_name);
  } else {
    if ((*e1)->d_type == DT_DIR)
      return -1;
    else
      return 1;
  }
}

// Decode %20 to space
void DecodeURL(char *path) {
  char *p = path;
  char buffer[1024] = {0};
  char *out = buffer;

  while (*p) {
    if (strncmp(p, "%20", 3) == 0) {
      *out++ = ' ';
      p += 3;
    } else {
      *out++ = *p++;
    }
  }
  *out = 0;
  strcpy(path, buffer);
}

void CreateHTML(const char *path, char **output) {
  struct dirent **result = NULL;

  Append(output, "<html>");
  int n = scandir(path, &result, NULL, Compare);
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      if (result[i]->d_type == DT_DIR) {
        // Folder link
        Append(output, "<a href=\"");
        Append(output, path);
        if (path[strlen(path) - 1] != '/')
          Append(output, "/");
        Append(output, result[i]->d_name);
        Append(output, "\">");
        Append(output, "<b>");
        Append(output, result[i]->d_name);
        Append(output, "</b>");
        Append(output, "</a>");
        Append(output, "<br>");
      } else {
        // File link with "?"
        Append(output, "<a href=\"");
        Append(output, path);
        if (path[strlen(path) - 1] != '/')
          Append(output, "/");
        Append(output, result[i]->d_name);
        Append(output, "?\">"); // add ?
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

void Send(int c, char *data, int len) {
  int sent = 0;
  while (sent < len) {
    int tmp = send(c, data + sent, len - sent, 0);
    if (tmp > 0)
      sent += tmp;
    else
      break;
  }
}

void *ClientThread(void *arg) {
  int c = *((int *)arg);
  free(arg);
  arg = NULL;

  char *requestBody = NULL;
  int bodyLen = 0;

  while (requestBody == NULL || strstr(requestBody, "\r\n\r\n") == NULL) {
    char chr;
    int r = recv(c, &chr, 1, 0);
    if (r > 0) {
      requestBody = (char *)realloc(requestBody, bodyLen + 1);
      requestBody[bodyLen] = chr;
      bodyLen++;
    } else
      break;
  }

  if (requestBody != NULL && strstr(requestBody, "\r\n\r\n") != NULL) {
    printf("%s", requestBody);

    char method[8] = {0};
    char path[1024] = {0};
    sscanf(requestBody, "%s%s", method, path);

    // Decode %20
    DecodeURL(path);

    if (strcmp(method, "GET") == 0) {
      if (strstr(path, "favicon.ico") != NULL) {
        FILE *f = fopen("favicon.ico", "rb");
        if (f != NULL) {
          fseek(f, 0, SEEK_END);
          int length = ftell(f);
          fseek(f, 0, SEEK_SET);
          char *icoData = (char *)calloc(length, 1);
          fread(icoData, 1, length, f);
          fclose(f);

          char ok[1024] = {0};
          sprintf(ok,
                  "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: "
                  "image/x-icon\r\n\r\n",
                  length);
          Send(c, ok, strlen(ok));
          Send(c, icoData, length);
          free(icoData);
        } else {
          char *notFound = "HTTP/1.1 404 NOT FOUND\r\n\r\n";
          Send(c, notFound, strlen(notFound));
        }
      } else {
        int isFile = 0;
        int pathLen = strlen(path);

        // check ? sign
        if (pathLen > 0 && path[pathLen - 1] == '?') {
          isFile = 1;
          path[pathLen - 1] = 0; // remove ?
        }

        if (isFile) {
          FILE *f = fopen(path, "rb");
          if (f != NULL) {
            fseek(f, 0, SEEK_END);
            int length = ftell(f);
            fseek(f, 0, SEEK_SET);

            char *data = (char *)malloc(length);
            fread(data, 1, length, f);
            fclose(f);

            char header[1024] = {0};
            sprintf(header,
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Length: %d\r\n"
                    "Content-Type: application/octet-stream\r\n"
                    "Content-Disposition: attachment\r\n\r\n",
                    length);

            Send(c, header, strlen(header));
            Send(c, data, length);
            free(data);
          } else {
            char *notFound = "HTTP/1.1 404 NOT FOUND\r\n\r\n";
            Send(c, notFound, strlen(notFound));
          }
        } else {
          // Folder view
          char *html = NULL;
          CreateHTML(path, &html);

          char ok[1024] = {0};
          sprintf(ok,
                  "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: "
                  "text/html\r\n\r\n",
                  (int)strlen(html));

          Send(c, ok, strlen(ok));
          Send(c, html, strlen(html));
          free(html);
        }
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
    printf("Server started: http://127.0.0.1:8888/\n");

    while (1) {
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
