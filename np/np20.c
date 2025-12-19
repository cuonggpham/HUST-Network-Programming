/*
 * CHUONG TRINH: HTTP Upload Server - Server ho tro upload file qua HTTP
 *
 * KIEN THUC LAP TRINH MANG:
 * - HTTP protocol: Request/Response header va body
 * - HTTP methods: GET (lay file/thu muc), POST (upload file)
 * - Content-Length: Header chi dinh kich thuoc body
 * - multipart/form-data: Dinh dang de upload file
 * - HTTP status codes: 200 OK, 404 Not Found, etc.
 *
 * KIEN THUC C ADVANCE:
 * - Parse HTTP request: Tach method, path, headers
 * - strstr(): Tim chuoi con trong HTTP header
 * - Dynamic buffer: realloc() de doc HTTP request dong
 * - File I/O: Doc va ghi file binary
 */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

void Append(char **destination, const char *str)
{
    char *tmp = *destination;

    int oldlen = tmp != NULL ? strlen(tmp) : 0;
    int more = strlen(str) + 1;
    tmp = (char *)realloc(tmp, oldlen + more);
    memset(tmp + oldlen, 0, more);
    sprintf(tmp + strlen(tmp), "%s", str);
    *destination = tmp;
}

int Compare(const struct dirent **e1, const struct dirent **e2)
{
    if ((*e1)->d_type == (*e2)->d_type)
    {
        return strcmp((*e1)->d_name, (*e2)->d_name);
    }
    else
    {
        if ((*e1)->d_type == DT_DIR)
            return -1;
        else
            return 1;
    }
}

void CreateHTML(const char *path, char **output)
{
    struct dirent **result = NULL;

    Append(output, "<html>");
    Append(output, "<form method=\"post\" enctype=\"multipart/form-data\">");
    Append(output, "<label>File to upload: </label>");
    Append(output, "<input type=\"file\" id=\"file1\" name=\"file1\"/> <br>");
    Append(output, "<input type=\"submit\" value=\"Upload\"><br>");
    Append(output, "</form>");
    int n = scandir(path, &result, NULL, Compare);
    if (n > 0)
    {
        for (int i = 0; i < n; i++)
        {
            if (result[i]->d_type == DT_DIR)
            {
                Append(output, "<a href=\"");
                Append(output, path);
                if (path[strlen(path) - 1] != '/')
                {
                    Append(output, "/");
                }
                Append(output, result[i]->d_name);
                Append(output, "\">");
                Append(output, "<b>");
                Append(output, result[i]->d_name);
                Append(output, "</b>");
                Append(output, "</a>");
                Append(output, "<br>");
            }
            else
            {
                Append(output, "<a href=\"");
                Append(output, path);
                if (path[strlen(path) - 1] != '/')
                {
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

// Ham Send dam bao gui het tat ca du lieu
// send() co the khong gui het trong 1 lan goi
void Send(int c, char *data, int len)
{
    int sent = 0;
    while (sent < len) // Lap cho den khi gui het
    {
        // Gui phan con lai chua gui
        int tmp = send(c, data + sent, len - sent, 0);
        if (tmp > 0)
        {
            sent += tmp; // Cap nhat so byte da gui
        }
        else
            break; // Loi hoac ket noi dong
    }
}

void *ClientThread(void *arg)
{
    int c = *((int *)arg);
    free(arg);
    arg = NULL;
    char *requestBody = NULL;
    char *partBody = NULL;
    int bodyLen = 0;
    int partLen = 0;
    while (requestBody == NULL || strstr(requestBody, "\r\n\r\n") == NULL)
    {
        char chr;
        int r = recv(c, &chr, 1, 0);
        if (r > 0)
        {
            requestBody = (char *)realloc(requestBody, bodyLen + 1);
            requestBody[bodyLen] = chr;
            bodyLen++;
        }
        else
            break;
    }

    if (requestBody != NULL && strstr(requestBody, "\r\n\r\n") != NULL)
    {
        int contentRead = 0;
        int contentLength = 0;
        char *ctLen = strstr(requestBody, "Content-Length: ");
        if (ctLen != NULL)
        {
            sscanf(ctLen + strlen("Content-Length: "), "%d", &contentLength);
        }
        printf("%s", requestBody);
        char method[8] = {0};
        char path[1024] = {0};
        sscanf(requestBody, "%s%s", method, path);
        char tmp[1024] = {0};
        while (strstr(path, "%20") != NULL)
        {
            memset(tmp, 0, sizeof(tmp));
            char *space = strstr(path, "%20");
            strncpy(tmp, path, space - path);
            sprintf(tmp + strlen(tmp), " %s", space + 3);
            memcpy(path, tmp, sizeof(tmp));
        }

        if (strcmp(method, "GET") == 0)
        {
            if (strstr(path, "favicon.ico") != NULL)
            {
                FILE *f = fopen("favicon.ico", "rb");
                if (f != NULL)
                {
                    fseek(f, 0, SEEK_END);
                    int length = ftell(f);
                    fseek(f, 0, SEEK_SET);
                    char *icoData = (char *)calloc(length, 1);
                    fread(icoData, 1, length, f);
                    fclose(f);
                    char ok[1024] = {0};
                    sprintf(ok, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: image/x-icon\r\n\r\n", length);
                    Send(c, ok, strlen(ok));
                    Send(c, icoData, length);
                }
                else
                {
                    char *notFound = "HTTP/1.1 404 NOT FOUND\r\n\r\n";
                    Send(c, notFound, strlen(notFound));
                }
            }
            else
            {
                if (path[strlen(path) - 1] != '?')
                {
                    char *html = NULL;
                    CreateHTML(path, &html);
                    char ok[1024] = {0};
                    sprintf(ok, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", (int)strlen(html));
                    Send(c, ok, strlen(ok));
                    Send(c, html, strlen(html));
                }
                else
                {
                    path[strlen(path) - 1] = 0;
                    char mime_type[1024] = {0};

                    if (strstr(path, ".c") != NULL)
                    {
                        sprintf(mime_type, "text/plain");
                    }
                    else if (strstr(path, ".jpg") != NULL)
                    {
                        sprintf(mime_type, "image/jpeg");
                    }
                    else if (strstr(path, ".png") != NULL)
                    {
                        sprintf(mime_type, "image/png");
                    }
                    else if (strstr(path, ".pdf") != NULL)
                    {
                        sprintf(mime_type, "application/pdf");
                    }
                    else if (strstr(path, ".mp4") != NULL)
                    {
                        sprintf(mime_type, "video/mp4");
                    }
                    else
                    {
                        sprintf(mime_type, "application/octet-stream");
                    }

                    char response[1024] = {0};
                    FILE *f = fopen(path, "rb");
                    if (f != NULL)
                    {
                        fseek(f, 0, SEEK_END);
                        int len = ftell(f);
                        fseek(f, 0, SEEK_SET);
                        sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: %s\r\n\r\n", len, mime_type);
                        Send(c, response, strlen(response));
                        int block_size = 1024 * 1024;
                        char *data = (char *)calloc(block_size, 1);
                        int read = 0;
                        while (read < len)
                        {
                            int tmpr = fread(data, 1, block_size, f);
                            Send(c, data, tmpr);
                            read += tmpr;
                        }
                        free(data);
                        data = NULL;
                    }
                    else
                    {
                        sprintf(response, "HTTP/1.1 404 NOT FOUND\r\n\r\n");
                        Send(c, response, strlen(response));
                    }
                    fclose(f);
                }
            }
        }
        else if (strcmp(method, "POST") == 0)
        {
            while (partBody == NULL || strstr(partBody, "\r\n\r\n") == NULL)
            {
                char chr;
                int r = recv(c, &chr, 1, 0);
                if (r > 0)
                {
                    partBody = (char *)realloc(partBody, partLen + 1);
                    partBody[partLen] = chr;
                    partLen++;
                    contentRead++;
                }
                else
                    break;
            }
            if (strstr(partBody, "\r\n\r\n") != NULL)
            {
                char *filename = strstr(partBody, "filename=");
                if (filename != NULL)
                {
                    char name[1024] = {0};
                    sscanf(filename + 10, "%s", name);
                    name[strlen(name) - 1] = 0;
                    char fullpath[2048] = {0};
                    if (path[strlen(path) - 1] == '/')
                        sprintf(fullpath, "%s%s", path, name);
                    else
                        sprintf(fullpath, "%s/%s", path, name);
                    FILE *f = fopen(fullpath, "wb");
                    if (f != NULL)
                    {
                        while (contentRead < contentLength)
                        {
                            char data[1024] = {0};
                            int r = recv(c, data, sizeof(data), 0);
                            if (r > 0)
                            {
                                fwrite(data, 1, r, f);
                                contentRead += r;
                            }
                            else
                                break;
                        }
                        fclose(f);
                        char *response = "HTTP/1.1 200 OK\r\n\r\n";
                        Send(c, response, strlen(response));
                    }
                    else
                    {
                        char *response = "HTTP/1.1 500 INTERNAL SERVER ERROR\r\n\r\n";
                        Send(c, response, strlen(response));
                    }
                }
                else
                {
                    char *response = "HTTP/1.1 400 BAD REQUEST\r\n\r\n";
                    Send(c, response, strlen(response));
                }
            }
            else
            {
                char *response = "HTTP/1.1 400 BAD REQUEST\r\n\r\n";
                Send(c, response, strlen(response));
            }
        }
    }

    close(c);
}

int main()
{
    SOCKADDR_IN saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    saddr.sin_port = htons(8888);
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (bind(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR)) == 0)
    {
        listen(s, 10);
        while (0 == 0)
        {
            int c = accept(s, NULL, 0);
            pthread_t tid = 0;
            int *arg = (int *)calloc(1, sizeof(int));
            *arg = c;
            pthread_create(&tid, NULL, ClientThread, arg);
        }
    }
    else
    {
        printf("Failed to bind\n");
    }
}