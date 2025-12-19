#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>

#define PORT 8080
#define ROOT "."
#define BUF 4096


void *handle_client(void *arg) {
    int sock = *(int*)arg;
    free(arg);

    char req[BUF];
    int n = recv(sock, req, sizeof(req)-1, 0);
    if (n <= 0) { close(sock); return NULL; }
    req[n] = '\0';

    char path[256] = "/";
    if (sscanf(req, "GET %255s", path) != 1) {
        strcpy(path, "/");
    }

    if (strcmp(path, "/favicon.ico") == 0) {
        FILE *f = fopen("favicon.ico", "rb");
        if (!f) { close(sock); return NULL; }

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);

        char header[256];
        int h = snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: image/x-icon\r\n"
            "Content-Length: %ld\r\n\r\n",
            size
        );
        send(sock, header, h, 0);

        char buf[1024];
        size_t r;
        while ((r = fread(buf, 1, sizeof(buf), f)) > 0)
            send(sock, buf, r, 0);
        fclose(f);
        close(sock);
        return NULL;
    }

    DIR *dir = opendir(ROOT);
    if (!dir) {
        close(sock);
        return NULL;
    }

    char html[BUF * 4];
    int off = 0;

    off += sprintf(html + off,
        "<html><head>"
        "<title>Directory</title>"
        "<link rel='icon' href='/favicon.ico'>"
        "</head><body>"
        "<h1>Directory Listing</h1><ul>"
    );

    struct dirent *e;

    rewinddir(dir);
    while ((e = readdir(dir))) {
        if (e->d_type == DT_DIR && e->d_name[0] != '.') {
            off += sprintf(html + off,
                "<li><b><a href='/%s'>%s/</a></b></li>",
                e->d_name, e->d_name
            );
        }
    }

    rewinddir(dir);
    while ((e = readdir(dir))) {
        if (e->d_type != DT_DIR && e->d_name[0] != '.') {
            off += sprintf(html + off,
                "<li><i><a href='/%s'>%s</a></i></li>",
                e->d_name, e->d_name
            );
        }
    }

    off += sprintf(html + off, "</ul></body></html>");
    closedir(dir);

    char header[256];
    int h = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n\r\n",
        off
    );

    send(sock, header, h, 0);
    send(sock, html, off, 0);

    close(sock);
    return NULL;
}

int main() {
    int server = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(PORT)
    };

    bind(server, (struct sockaddr*)&addr, sizeof(addr));
    listen(server, 10);

    printf("Server running: http://localhost:%d\n", PORT);

    while (1) {
        int *client = malloc(sizeof(int));
        *client = accept(server, NULL, NULL);

        pthread_t t;
        pthread_create(&t, NULL, handle_client, client);
        pthread_detach(t);
    }

    return 0;
}
