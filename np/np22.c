/*
 * CHUONG TRINH: FTP Server - File Transfer Protocol Server
 *
 * KIEN THUC LAP TRINH MANG:
 * - FTP: Giao thuc truyen file co 2 ket noi (control va data)
 * - Control connection: Port 21 (hoac 8888) de gui lenh
 * - Data connection: Port dong de truyen file/list
 * - PASV mode: Server mo port cho client ket noi (passive mode)
 * - FTP commands: USER, PASS, LIST, CWD, PWD, RETR, STOR, etc.
 * - Response codes: 220, 331, 230, 150, 226, etc.
 *
 * KIEN THUC C ADVANCE:
 * - Two-connection architecture: Control channel va data channel
 * - Command parsing: Parse FTP command text
 * - Dynamic port allocation: Tang port sau moi PASV command
 * - Path manipulation: Quan ly thu muc hien tai
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
#include <sys/select.h>

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

int g_clientSockets[1024] = {0};
int g_clientCount = 0;
char rootPath[1024] = {0}; // Thu muc hien tai cua client
int dataSocket = -1;       // Socket cho data connection
int pasvPort = 10000;      // Port bat dau cho PASV mode

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

void CreateLIST(const char *path, char **output)
{
    struct dirent **result = NULL;

    int n = scandir(path, &result, NULL, Compare);
    if (n > 0)
    {
        for (int i = 0; i < n; i++)
        {
            if (result[i]->d_type == DT_DIR)
            {
                Append(output, "drwxrwxrwx 0 0 0 0 Dec 15 2025 ");
                Append(output, result[i]->d_name);
                Append(output, "\r\n");
            }
            else
            {
                Append(output, "-rwxrwxrwx 0 0 0 0 Dec 15 2025 ");
                Append(output, result[i]->d_name);
                Append(output, "\r\n");
            }
            free(result[i]);
            result[i] = NULL;
        }
    }
}

void Send(int c, char *data, int len)
{
    int sent = 0;
    while (sent < len)
    {
        int tmp = send(c, data + sent, len - sent, 0);
        if (tmp > 0)
        {
            sent += tmp;
        }
        else
            break;
    }
}

int main()
{
    strcpy(rootPath, "/");
    SOCKADDR_IN saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8888);
    saddr.sin_addr.s_addr = 0;
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (bind(s, (SOCKADDR *)&saddr, sizeof(saddr)) == 0)
    {
        listen(s, 10);
        fd_set read;
        while (0 == 0)
        {
            FD_ZERO(&read);
            FD_SET(s, &read);
            for (int i = 0; i < g_clientCount; i++)
            {
                FD_SET(g_clientSockets[i], &read);
            }
            int n = select(FD_SETSIZE, &read, NULL, NULL, NULL);
            if (n > 0)
            {
                if (FD_ISSET(s, &read))
                {
                    int c = accept(s, NULL, NULL);
                    printf("A new client connected.\n");
                    g_clientSockets[g_clientCount++] = c;
                    char *response = "220 MY FTP SERVER\r\n";
                    send(c, response, strlen(response), 0);
                }
                for (int i = 0; i < g_clientCount; i++)
                {
                    if (FD_ISSET(g_clientSockets[i], &read))
                    {
                        char data[1024] = {0};
                        int c = g_clientSockets[i];
                        recv(c, data, sizeof(data) - 1, 0);
                        if (strncmp(data, "USER", 4) == 0)
                        {
                            char *response = "331 OK\r\n";
                            send(c, response, strlen(response), 0);
                        }
                        else if (strncmp(data, "PASS", 4) == 0)
                        {
                            char *response = "230 OK\r\n";
                            send(c, response, strlen(response), 0);
                        }
                        else if (strncmp(data, "SYST", 4) == 0)
                        {
                            char *response = "215 UNIX Type: L8\r\n";
                            send(c, response, strlen(response), 0);
                        }
                        else if (strncmp(data, "HELP", 4) == 0)
                        {
                            char *response = "202 Command not implemented\r\n";
                            send(c, response, strlen(response), 0);
                        }
                        else if (strncmp(data, "FEAT", 4) == 0)
                        {
                            char *response = "211-Features:\r\n EPRT\r\n EPSV\r\n MDTM\r\n PASV\r\n REST STREAM\r\n SIZE\r\n TVFS\r\n211 End\r\n";
                            send(c, response, strlen(response), 0);
                        }
                        else if (strncmp(data, "OPTS", 4) == 0)
                        {
                            char *response = "200 OK\r\n";
                            send(c, response, strlen(response), 0);
                        }
                        else if (strncmp(data, "PWD", 3) == 0)
                        {
                            char response[2048] = {0};
                            sprintf(response, "257 \"%s\" OK\r\n", rootPath);
                            send(c, response, strlen(response), 0);
                        }
                        else if (strncmp(data, "TYPE", 4) == 0)
                        {
                            char *response = "200 OK\r\n";
                            send(c, response, strlen(response), 0);
                        }
                        else if (strncmp(data, "PASV", 4) == 0)
                        {
                            // PASV (Passive mode): Server mo port cho client ket noi
                            // Response format: 227 OK (h1,h2,h3,h4,p1,p2)
                            // IP = h1.h2.h3.h4, Port = p1*256 + p2
                            char response[1024] = {0};
                            sprintf(response, "227 OK (172,20,38,136,%d,%d)\r\n", (pasvPort >> 8) & 0xFF, pasvPort & 0xFF);
                            send(c, response, strlen(response), 0);

                            // Tao socket moi cho data connection
                            SOCKADDR_IN saddr;
                            saddr.sin_family = AF_INET;
                            saddr.sin_port = htons(pasvPort);
                            saddr.sin_addr.s_addr = 0;
                            int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

                            if (bind(s, (SOCKADDR *)&saddr, sizeof(saddr)) == 0)
                            {
                                listen(s, 10);
                                // Cho doi client ket noi vao data port
                                dataSocket = accept(s, NULL, NULL);
                                close(s);
                                printf("Data connection established.\r\n");
                            }
                            else
                            {
                                printf("Failed to bind to PASV port.\r\n");
                                close(s);
                            }

                            pasvPort++; // Tang port cho lan PASV tiep theo
                        }
                        else if (strncmp(data, "LIST", 4) == 0)
                        {
                            if (dataSocket > 0)
                            {
                                char *response1 = "150 OK\r\n";
                                send(c, response1, strlen(response1), 0);
                                char *list = NULL;
                                CreateLIST(rootPath, &list);
                                Send(dataSocket, list, strlen(list));
                                char *response2 = "226 OK\r\n";
                                send(c, response2, strlen(response2), 0);
                                usleep(100000);
                                close(dataSocket);
                                dataSocket = -1;
                                printf("Data sent.\r\n");
                            }
                            else
                            {
                                char *response = "425 Failed to open data connection\r\n";
                                send(c, response, strlen(response), 0);
                            }
                        }
                        else if (strncmp(data, "QUIT", 4) == 0)
                        {
                            char *response = "202 Command not implemented\r\n";
                            send(c, response, strlen(response), 0);
                        }
                        else if (strncmp(data, "CWD", 3) == 0)
                        {
                            char *folder = data + 4;
                            while (folder[strlen(folder) - 1] == '\r' || folder[strlen(folder) - 1] == '\n')
                            {
                                folder[strlen(folder) - 1] = 0;
                            }
                            if (rootPath[strlen(rootPath) - 1] == '/')
                            {
                                sprintf(rootPath + strlen(rootPath), "%s", folder);
                            }
                            else
                            {
                                sprintf(rootPath + strlen(rootPath), "/%s", folder);
                            }
                            char *response = "200 OK\r\n";
                            send(c, response, strlen(response), 0);
                        }
                        else if (strncmp(data, "CDUP", 4) == 0)
                        {
                            while (rootPath[strlen(rootPath) - 1] != '/')
                            {
                                rootPath[strlen(rootPath) - 1] = 0;
                            }
                            if (strcmp(rootPath, "/") != 0)
                            {
                                rootPath[strlen(rootPath) - 1] = 0;
                            }
                            char *response = "200 OK\r\n";
                            send(c, response, strlen(response), 0);
                        }
                        else
                        {
                            char *response = "202 Command not implemented\r\n";
                            send(c, response, strlen(response), 0);
                        }
                    }
                }
            }
        }
    }
    else
        printf("Failed to bind.\n");

    close(s);
}