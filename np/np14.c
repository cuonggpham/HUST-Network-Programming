/*
 * CHUONG TRINH: Signal Handling - Inter-Process Communication
 *
 * KIEN THUC LAP TRINH MANG:
 * - Chat server: Broadcast tin nhan tu mot client den tat ca client khac
 * - Inter-process communication: Child gui signal cho parent
 * - Dung socket noi bo (localhost) de truyen du lieu giua process
 *
 * KIEN THUC C ADVANCE:
 * - signal(): Dang ky ham xu ly tin hieu (signal handler)
 * - SIGINT: Signal khi nhan Ctrl+C
 * - SIGCHLD: Signal khi child process ket thuc
 * - Custom signal: SIGRTMIN+1 la real-time signal tu dinh nghia
 * - sigqueue(): Gui signal voi du lieu kem theo
 * - getpid(): Lay Process ID cua tien trinh hien tai
 * - union sigval: Cau truc de truyen du lieu voi signal
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

// Dinh nghia custom signal (real-time signal)
#define MY_SIGNAL SIGRTMIN + 1

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

// Bien toan cuc de quan ly client sockets
int g_sockets[1024] = {0}; // Mang luu socket cua cac client
int g_count = 0;           // So luong client
int s = 0;                 // Server socket
int parentPID = 0;         // PID cua parent process

// Ham xu ly signal - Duoc goi khi co signal gui den process
// sig la loai signal nhan duoc
void sig_handler(int sig)
{
    // Xu ly SIGINT (Ctrl+C)
    if (sig == SIGINT)
    {
        printf("Ctrl+C!\n");
        close(s);
        exit(0);
    }

    // Xu ly SIGCHLD (child process terminated)
    // Signal nay duoc gui khi mot child process ket thuc
    if (sig == SIGCHLD)
    {
        printf("A child process has been terminated!\n");
    }

    // Xu ly custom signal tu child process
    // Child gui signal nay khi nhan duoc message can broadcast
    if (sig == MY_SIGNAL)
    {
        printf("A signal from a child process\n");

        // Tao socket de ket noi voi child process va nhan du lieu
        // Dung localhost (127.0.0.1) cho IPC qua socket
        int tmp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        SOCKADDR_IN tmpAddr;
        tmpAddr.sin_family = AF_INET;
        tmpAddr.sin_port = htons(9999); // Port tam thoi cua child
        tmpAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        connect(tmp, (SOCKADDR *)&tmpAddr, sizeof(SOCKADDR));

        // Nhan du lieu tu child: "socket_id message"
        char buffer[1024] = {0};
        recv(tmp, buffer, sizeof(buffer) - 1, 0);
        close(tmp);

        // Parse socket ID cua nguoi gui
        sscanf(buffer, "%d", &tmp);

        // Broadcast message den tat ca client tru nguoi gui
        for (int i = 0; i < g_count; i++)
        {
            if (g_sockets[i] != tmp)
            {
                // strstr(buffer, " ") tim dau cach, +1 de lay phan message
                char *data = strstr(buffer, " ") + 1;
                send(g_sockets[i], data, strlen(data), 0);
            }
        }
    }
}

int main()
{
    // Luu PID cua parent de child co the gui signal ve
    parentPID = getpid();

    // signal() dang ky ham xu ly cho cac signal
    // Tham so 1: Loai signal, Tham so 2: Ham xu ly
    signal(SIGINT, sig_handler);    // Xu ly Ctrl+C
    signal(SIGCHLD, sig_handler);   // Xu ly child termination
    signal(MY_SIGNAL, sig_handler); // Xu ly custom signal

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8888);
    saddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    if (bind(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR)) == 0)
    {
        listen(s, 10);
        while (0 == 0)
        {
            SOCKADDR_IN caddr;
            int clen = sizeof(SOCKADDR_IN);
            printf("Parent: waiting for connection!\n");
            int d = accept(s, (SOCKADDR *)&caddr, &clen);
            printf("Parent: one more client connected!\n");
            g_sockets[g_count++] = d;
            if (fork() == 0)
            {
                close(s);
                while (0 == 0)
                {
                    char buffer[1024] = {0};
                    printf("Child: waiting for data\n");
                    int received = recv(d, buffer, sizeof(buffer) - 1, 0);
                    if (received > 0)
                    {
                        printf("Child: received %d bytes: %s\n", received, buffer);
                        int tmp1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                        SOCKADDR_IN tmpAddr;
                        tmpAddr.sin_family = AF_INET;
                        tmpAddr.sin_port = htons(9999);
                        tmpAddr.sin_addr.s_addr = 0;
                        if (bind(tmp1, (SOCKADDR *)&tmpAddr, sizeof(tmpAddr)) == 0)
                        {
                            listen(tmp1, 10);

                            union sigval v;
                            sigqueue(parentPID, MY_SIGNAL, v);

                            int tmpLen = sizeof(SOCKADDR);
                            int tmp2 = accept(tmp1, (SOCKADDR *)&tmpAddr, &tmpLen);
                            char data[2048] = {0};
                            sprintf(data, "%d %s", d, buffer);
                            send(tmp2, data, strlen(data), 0);
                            sleep(1);
                            close(tmp2);
                            close(tmp1);
                            printf("Child: socket closed\n");
                        }
                        else
                            printf("Child: failed to bind\n");
                    }
                    else
                    {
                        printf("Child: disconnected!\n");
                        exit(0);
                    }
                }
            }
        }
    }
    else
    {
        printf("Failed to bind!\n");
        close(s);
    }
}