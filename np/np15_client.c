/*
 * CHUONG TRINH: P2P File Transfer Client - Truyen file truc tiep
 *
 * KIEN THUC LAP TRINH MANG:
 * - Peer-to-peer architecture: Client vua la client vua la server
 * - UDP broadcast de tim client khac
 * - TCP de truyen file giua 2 client
 * - File transfer protocol: Gui kich thuoc file truoc, sau do gui du lieu
 *
 * KIEN THUC C ADVANCE:
 * - Multi-process: Fork de xu ly dong thoi nhan va gui
 * - Signal handling: SIGCHLD de quan ly child process
 * - waitpid(): Thu hoi zombie process
 * - WNOHANG flag: Khong block khi waitpid()
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

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

// typedef de tao kieu du lieu moi: STRING la mang char 1024 phan tu
typedef char STRING[1024];

// Thong tin cua client nay
char *MYIP = "172.20.38.136";
int MYPORT = 10000;
char *MYNAME = "UNAME_001";

void sig_handler(int sig)
{
    if (sig == SIGCHLD)
    {
        int statloc;
        pid_t pid;
        while ((pid = waitpid(-1, &statloc, WNOHANG)) > 0)
        {
            printf("A child process has been terminated: %d\n", pid);
        }
    }
}

int main()
{
    signal(SIGCHLD, sig_handler);

    if (fork() == 0)
    {
        int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        SOCKADDR_IN saddr, caddr;
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(7000);
        saddr.sin_addr.s_addr = inet_addr("0.0.0.0");
        if (bind(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR)) == 0)
        {
            while (0 == 0)
            {
                char buffer[65546] = {0};
                int clen = sizeof(SOCKADDR);
                int received = recvfrom(s, buffer, sizeof(buffer) - 1, 0, (SOCKADDR *)&caddr, &clen);
                if (received > 0 && strncmp(buffer, "SEND", 4) == 0)
                {
                    char command[1024] = {0};
                    char file[1024] = {0};
                    char name[1024] = {0};
                    sscanf(buffer, "%s%s%s", command, file, name);
                    if (strcmp(name, MYNAME) == 0)
                    {
                        memset(buffer, 0, sizeof(buffer));
                        sprintf(buffer, "%s %d", MYIP, MYPORT);
                        sendto(s, buffer, strlen(buffer), 0, (SOCKADDR *)&caddr, clen);
                        int ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                        saddr.sin_family = AF_INET;
                        saddr.sin_port = htons(MYPORT);
                        saddr.sin_addr.s_addr = inet_addr("0.0.0.0");
                        if (bind(ls, (SOCKADDR *)&saddr, sizeof(SOCKADDR)) == 0)
                        {
                            listen(ls, 10);
                            int ds = accept(ls, NULL, 0);
                            int flen = 0;
                            recv(ds, &flen, 4, 0);
                            int bs = 2 * 1024 * 1024;
                            char *buffer = (char *)calloc(bs, 1);
                            int received = 0;
                            FILE *f = fopen("out.dat", "rb");
                            while (received < flen)
                            {
                                int tmp = recv(ds, buffer, sizeof(buffer), 0);
                                if (tmp > 0)
                                {
                                    fwrite(buffer, tmp, 1, f);
                                    received += tmp;
                                }
                                else
                                    break;
                            }
                            fclose(f);
                            free(buffer);
                            close(ds);
                            if (received == flen)
                            {
                                printf("Received file OK\n");
                            }
                            else
                                printf("Failed to receive a file\n");
                        }
                        else
                        {
                            printf("Failed to bind!\n");
                            close(ls);
                        }
                    }
                }
            }
        }
        exit(0);
    }

    int bs = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int on = 1;
    setsockopt(bs, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
    SOCKADDR_IN raddr, laddr, saddr;
    raddr.sin_family = AF_INET;
    raddr.sin_port = htons(5000);
    raddr.sin_addr.s_addr = inet_addr("255.255.255.255");
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(6000);
    laddr.sin_addr.s_addr = inet_addr("255.255.255.255");
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(7000);
    saddr.sin_addr.s_addr = inet_addr("255.255.255.255");

    while (0 == 0)
    {
        // Broadcast REG COMMAND
        char buffer[65536] = {0};
        sprintf(buffer, "REG %s", MYNAME);
        sendto(bs, buffer, strlen(buffer), 0, (SOCKADDR *)&raddr, sizeof(SOCKADDR));

        // Wait for LIST or SEND from keyboard
        memset(buffer, 0, sizeof(buffer));
        fgets(buffer, sizeof(buffer) - 1, stdin);
        if (strncmp(buffer, "LIST", 4) == 0)
        {
            // Broadcast LIST COMMAND
            sendto(bs, buffer, strlen(buffer), 0, (SOCKADDR *)&laddr, sizeof(SOCKADDR));
            memset(buffer, 0, sizeof(buffer));
            int received = recvfrom(bs, buffer, sizeof(buffer) - 1, 0, NULL, 0);
            printf("Received %d bytes from the server.\n", received);
            printf("%s\n", buffer);
        }

        if (strncmp(buffer, "SEND", 4) == 0)
        {
            char command[1024] = {0};
            char file[1024] = {0};
            char name[1024] = {0};
            sscanf(buffer, "%s%s%s", command, file, name);
            // Broadcast SEND COMMAND
            sendto(bs, buffer, strlen(buffer), 0, (SOCKADDR *)&saddr, sizeof(SOCKADDR));
            memset(buffer, 0, sizeof(buffer));
            recvfrom(bs, buffer, strlen(buffer) - 1, 0, NULL, 0);
            usleep(100000);
            char ip[1024] = {0};
            int port = 0;
            sscanf(buffer, "%s%d", ip, &port);
            int tcps = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            SOCKADDR_IN taddr;
            taddr.sin_family = AF_INET;
            taddr.sin_port = htons(port);
            taddr.sin_addr.s_addr = inet_addr(ip);
            if (connect(tcps, (SOCKADDR *)&taddr, sizeof(SOCKADDR)) == 0)
            {
                FILE *f = fopen(file, "rb");
                fseek(f, SEEK_END, 0);
                int flen = ftell(f);
                fseek(f, SEEK_SET, 0);
                send(tcps, &flen, 4, 0);
                int sent = 0;
                int bs = 10 * 1024 * 1024;
                char *data = (char *)calloc(bs, 1);
                while (sent < flen)
                {
                    int r = fread(data, 1, sizeof(data), f);
                    int tmpSent = 0;
                    while (tmpSent < r)
                    {
                        int tmp = send(tcps, data + tmpSent, r - tmpSent, 0);
                        if (tmp > 0)
                        {
                            tmpSent += tmp;
                        }
                        else
                            break;
                    }
                    sent += tmpSent;
                }
                fclose(f);
                free(data);
            }
            else
            {
                printf("Failed to connect.\n");
                close(tcps);
            }
        }
    }
}