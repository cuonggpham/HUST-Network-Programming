/*
 * CHUONG TRINH: Multi-thread Command Execution Server
 *
 * KIEN THUC LAP TRINH MANG:
 * - Multi-thread server thuc thi lenh shell
 * - Dung mutex de tranh xung dot khi ghi file
 *
 * KIEN THUC C ADVANCE:
 * - Mutex trong multi-thread: Bao ve tai nguyen chung (file)
 * - Critical section: system() va file I/O can duoc bao ve
 * - Thread-safe: Dam bao code chay dung khi co nhieu thread
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

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

// Mutex de bao ve viec ghi file out.txt
// Nhieu thread cung ghi vao 1 file se bi xung dot neu khong co mutex
pthread_mutex_t *pMutex = NULL;

void *ClientThread(void *arg)
{
    int c = *((int *)arg);
    while (0 == 0)
    {
        char buffer[1024] = {0};
        int r = recv(c, buffer, sizeof(buffer) - 1, 0);
        if (r > 0)
        {
            while (buffer[strlen(buffer) - 1] == '\r' || buffer[strlen(buffer) - 1] == '\n')
            {
                buffer[strlen(buffer) - 1] = 0;
            }
            char command[2048] = {0};
            sprintf(command, "%s > out.txt", buffer);
            pthread_mutex_lock(pMutex);
            system(command);
            FILE *f = fopen("out.txt", "rt");
            while (!feof(f))
            {
                char line[1024] = {0};
                fgets(line, sizeof(line) - 1, f);
                send(c, line, strlen(line), 0);
            }
            fclose(f);
            pthread_mutex_unlock(pMutex);
        }
        else
            break;
    }
    close(c);
}
int main()
{
    pMutex = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
    pthread_mutex_init(pMutex, NULL);

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
            pthread_t tid;
            int *arg = (int *)calloc(1, sizeof(int));
            *arg = c;
            pthread_create(&tid, NULL, ClientThread, arg);
        }
    }
    else
    {
        printf("Failed to bind.\n");
        close(s);
    }
    pthread_mutex_destroy(pMutex);
}