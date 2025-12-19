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

// Decode %20 to space
void DecodeURL(char *path)
{
    char *p = path;
    char buffer[1024] = {0};
    char *out = buffer;

    while (*p)
    {
        if (strncmp(p, "%20", 3) == 0)
        {
            *out++ = ' ';
            p += 3;
        }
        else
        {
            *out++ = *p++;
        }
    }
    *out = 0;
    strcpy(path, buffer);
}

void CreateHTML(const char *path, char **output)
{
    struct dirent **result = NULL;

    Append(output, "<html>");
    Append(output, "<head><meta charset='utf-8'></head>");
    Append(output, "<body>");

    // Add upload form
    Append(output, "<h2>Upload File</h2>");
    Append(output, "<form method='POST' enctype='multipart/form-data'>");
    Append(output, "<input type='file' name='file' required>");
    Append(output, "<input type='submit' value='Upload'>");
    Append(output, "</form>");
    Append(output, "<hr>");
    Append(output, "<h2>Files in ");
    Append(output, path);
    Append(output, "</h2>");

    int n = scandir(path, &result, NULL, Compare);
    if (n > 0)
    {
        for (int i = 0; i < n; i++)
        {
            if (result[i]->d_type == DT_DIR)
            {
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
            }
            else
            {
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

    Append(output, "</body>");
    Append(output, "</html>");
}

void Send(int c, char *data, int len)
{
    int sent = 0;
    while (sent < len)
    {
        int tmp = send(c, data + sent, len - sent, 0);
        if (tmp > 0)
            sent += tmp;
        else
            break;
    }
}

// Parse multipart/form-data to extract file
int ParseMultipart(char *body, int bodyLen, const char *boundary, char **filename, char **fileData, int *fileSize)
{
    char boundaryStart[256];
    sprintf(boundaryStart, "--%s\r\n", boundary);

    char *start = strstr(body, boundaryStart);
    if (start == NULL)
        return 0;

    start += strlen(boundaryStart);

    // Find Content-Disposition header
    char *dispHeader = strstr(start, "Content-Disposition:");
    if (dispHeader == NULL)
        return 0;

    // Extract filename
    char *fnameStart = strstr(dispHeader, "filename=\"");
    if (fnameStart == NULL)
        return 0;
    fnameStart += 10; // skip filename="

    char *fnameEnd = strchr(fnameStart, '\"');
    if (fnameEnd == NULL)
        return 0;

    int fnameLen = fnameEnd - fnameStart;
    *filename = (char *)malloc(fnameLen + 1);
    memcpy(*filename, fnameStart, fnameLen);
    (*filename)[fnameLen] = 0;

    // Find the start of file content (after \r\n\r\n)
    char *contentStart = strstr(start, "\r\n\r\n");
    if (contentStart == NULL)
    {
        free(*filename);
        *filename = NULL;
        return 0;
    }
    contentStart += 4;

    // Find the end boundary
    char boundaryEnd[256];
    sprintf(boundaryEnd, "\r\n--%s", boundary);

    char *contentEnd = strstr(contentStart, boundaryEnd);
    if (contentEnd == NULL)
    {
        free(*filename);
        *filename = NULL;
        return 0;
    }

    *fileSize = contentEnd - contentStart;
    *fileData = (char *)malloc(*fileSize);
    memcpy(*fileData, contentStart, *fileSize);

    return 1;
}

// Verify file integrity
int VerifyFile(const char *path, const char *uploadedData, int uploadedSize)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return 0;

    fseek(f, 0, SEEK_END);
    int savedSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (savedSize != uploadedSize)
    {
        fclose(f);
        return 0;
    }

    char *savedData = (char *)malloc(savedSize);
    fread(savedData, 1, savedSize, f);
    fclose(f);

    int result = (memcmp(savedData, uploadedData, savedSize) == 0);
    free(savedData);

    return result;
}

void *ClientThread(void *arg)
{
    int c = *((int *)arg);
    free(arg);
    arg = NULL;

    char *requestBody = NULL;
    int bodyLen = 0;
    int contentLength = 0;
    char boundary[256] = {0};
    char method[8] = {0};
    char path[1024] = {0};
    int headerEnd = 0;

    // Read request header
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
        headerEnd = strstr(requestBody, "\r\n\r\n") - requestBody + 4;

        // Parse method and path
        sscanf(requestBody, "%s%s", method, path);
        DecodeURL(path);

        // Parse Content-Length
        char *clHeader = strstr(requestBody, "Content-Length:");
        if (clHeader != NULL)
        {
            sscanf(clHeader, "Content-Length: %d", &contentLength);
        }

        // Parse boundary for multipart
        char *ctHeader = strstr(requestBody, "Content-Type:");
        if (ctHeader != NULL)
        {
            char *boundaryStart = strstr(ctHeader, "boundary=");
            if (boundaryStart != NULL)
            {
                boundaryStart += 9;
                char *boundaryEnd = strstr(boundaryStart, "\r\n");
                if (boundaryEnd != NULL)
                {
                    int bLen = boundaryEnd - boundaryStart;
                    memcpy(boundary, boundaryStart, bLen);
                    boundary[bLen] = 0;
                }
            }
        }

        // If POST, read the rest of the body
        if (strcmp(method, "POST") == 0 && contentLength > 0)
        {
            int remainingBody = contentLength - (bodyLen - headerEnd);
            if (remainingBody > 0)
            {
                requestBody = (char *)realloc(requestBody, bodyLen + remainingBody);
                int totalRead = 0;
                while (totalRead < remainingBody)
                {
                    int r = recv(c, requestBody + bodyLen + totalRead, remainingBody - totalRead, 0);
                    if (r > 0)
                        totalRead += r;
                    else
                        break;
                }
                bodyLen += totalRead;
            }
        }

        printf("%s\n", method);

        if (strcmp(method, "POST") == 0 && strlen(boundary) > 0)
        {
            // Handle file upload
            char *filename = NULL;
            char *fileData = NULL;
            int fileSize = 0;

            if (ParseMultipart(requestBody, bodyLen, boundary, &filename, &fileData, &fileSize))
            {
                // Create full path
                char fullPath[2048];

                // If path is root "/", use current directory instead
                if (strcmp(path, "/") == 0)
                {
                    snprintf(fullPath, sizeof(fullPath), "./%s", filename);
                }
                else
                {
                    snprintf(fullPath, sizeof(fullPath), "%s/%s", path, filename);
                }

                printf("Trying to save: %s (size: %d)\n", fullPath, fileSize);

                // Save file
                FILE *f = fopen(fullPath, "wb");
                if (f != NULL)
                {
                    int written = fwrite(fileData, 1, fileSize, f);
                    fclose(f);

                    printf("Written %d bytes\n", written);

                    // Verify file
                    int verified = VerifyFile(fullPath, fileData, fileSize);

                    // Send response
                    char *html = NULL;
                    Append(&html, "<html><head><meta charset='utf-8'></head><body>");

                    if (verified)
                    {
                        Append(&html, "<h2>File uploaded successfully!</h2>");
                        Append(&html, "<p>Filename: ");
                        Append(&html, filename);
                        Append(&html, "</p>");
                        Append(&html, "<p>Size: ");
                        char sizeStr[64];
                        sprintf(sizeStr, "%d bytes", fileSize);
                        Append(&html, sizeStr);
                        Append(&html, "</p>");
                        Append(&html, "<p>✓ File integrity verified</p>");
                    }
                    else
                    {
                        Append(&html, "<h2>Upload failed!</h2>");
                        Append(&html, "<p>✗ File integrity check failed</p>");
                    }

                    Append(&html, "<p><a href='");
                    Append(&html, path);
                    Append(&html, "'>Back to folder</a></p>");
                    Append(&html, "</body></html>");

                    char header[1024];
                    sprintf(header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n",
                            (int)strlen(html));
                    Send(c, header, strlen(header));
                    Send(c, html, strlen(html));
                    free(html);
                }
                else
                {
                    printf("Failed to open file: %s\n", fullPath);
                    char *error = "HTTP/1.1 500 Internal Server Error\r\n\r\n<html><body><h2>Failed to save file</h2><p>Check path and permissions</p></body></html>";
                    Send(c, error, strlen(error));
                }

                free(filename);
                free(fileData);
            }
            else
            {
                char *error = "HTTP/1.1 400 Bad Request\r\n\r\n<html>Invalid multipart data</html>";
                Send(c, error, strlen(error));
            }
        }
        else if (strcmp(method, "GET") == 0)
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
                    free(icoData);
                }
                else
                {
                    char *notFound = "HTTP/1.1 404 NOT FOUND\r\n\r\n";
                    Send(c, notFound, strlen(notFound));
                }
            }
            else
            {
                int isFile = 0;
                int pathLen = strlen(path);

                // check ? sign
                if (pathLen > 0 && path[pathLen - 1] == '?')
                {
                    isFile = 1;
                    path[pathLen - 1] = 0; // remove ?
                }

                if (isFile)
                {
                    FILE *f = fopen(path, "rb");
                    if (f != NULL)
                    {
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
                    }
                    else
                    {
                        char *notFound = "HTTP/1.1 404 NOT FOUND\r\n\r\n";
                        Send(c, notFound, strlen(notFound));
                    }
                }
                else
                {
                    // Folder view
                    char *html = NULL;
                    CreateHTML(path, &html);

                    char ok[1024] = {0};
                    sprintf(ok, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n",
                            (int)strlen(html));

                    Send(c, ok, strlen(ok));
                    Send(c, html, strlen(html));
                    free(html);
                }
            }
        }
    }

    if (requestBody != NULL)
        free(requestBody);
    close(c);
    return NULL;
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
        printf("Server started: http://127.0.0.1:8888/\n");

        while (1)
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
