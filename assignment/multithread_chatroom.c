#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <errno.h>

#define PORT 8888
#define MAX_CLIENTS 1024
#define BUF_SIZE 1024

int clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

int server_sock = -1;
volatile sig_atomic_t stop_server = 0;

void handle_sigint(int sig)
{
    (void)sig;
    stop_server = 1;
    if (server_sock != -1)
    {
        shutdown(server_sock, SHUT_RDWR);
        close(server_sock);
        server_sock = -1;
    }
}

int add_client(int sockfd)
{
    pthread_mutex_lock(&clients_mutex);
    if (client_count >= MAX_CLIENTS)
    {
        pthread_mutex_unlock(&clients_mutex);
        return -1;
    }
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i] == 0)
        {
            clients[i] = sockfd;
            client_count++;
            pthread_mutex_unlock(&clients_mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return -1;
}

void remove_client(int sockfd)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i] == sockfd)
        {
            clients[i] = 0;
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void broadcast_message(int sender_sock, const char *msg, ssize_t len)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        int c = clients[i];
        if (c != 0 && c != sender_sock)
        {
            ssize_t sent = send(c, msg, len, 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *client_thread(void *arg)
{
    int client_sock = *((int *)arg);
    free(arg);

    pthread_detach(pthread_self());

    char buf[BUF_SIZE];
    while (1)
    {
        ssize_t r = recv(client_sock, buf, sizeof(buf), 0);
        if (r > 0)
        {
            char tmp = buf[r < sizeof(buf) ? r : sizeof(buf) - 1];
            if (r < sizeof(buf))
                buf[r] = '\0';
            else
                buf[sizeof(buf) - 1] = '\0';
            printf("Thread %lu: received %zd bytes: %s\n", (unsigned long)pthread_self(), r, buf);
            broadcast_message(client_sock, buf, r);
        }
        else if (r == 0)
        {
            printf("Thread %lu: client disconnected (socket %d)\n", (unsigned long)pthread_self(), client_sock);
            remove_client(client_sock);
            close(client_sock);
            break;
        }
        else
        {
            if (errno == EINTR)
                continue;
            printf("Thread %lu: recv error on socket %d: %s\n", (unsigned long)pthread_self(), client_sock, strerror(errno));
            remove_client(client_sock);
            close(client_sock);
            break;
        }
    }

    return NULL;
}

int main()
{
    for (int i = 0; i < MAX_CLIENTS; ++i)
        clients[i] = 0;

    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        close(server_sock);
        return 1;
    }

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(PORT);
    saddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        perror("bind");
        close(server_sock);
        return 1;
    }

    if (listen(server_sock, 10) < 0)
    {
        perror("listen");
        close(server_sock);
        return 1;
    }

    printf("Chat room server started on port %d\n", PORT);

    while (!stop_server)
    {
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);
        printf("Parent: waiting for connection...\n");
        int client_fd = accept(server_sock, (struct sockaddr *)&caddr, &clen);
        if (client_fd < 0)
        {
            if (stop_server)
                break;
            perror("accept");
            continue;
        }

        printf("Parent: client connected from %s:%d (socket %d)\n",
               inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port), client_fd);

        if (add_client(client_fd) != 0)
        {
            const char *msg = "Server is full. Try later.\n";
            send(client_fd, msg, strlen(msg), 0);
            close(client_fd);
            printf("Parent: rejected client (server full)\n");
            continue;
        }

        pthread_t tid;
        int *pclient = malloc(sizeof(int));
        if (!pclient)
        {
            perror("malloc");
            remove_client(client_fd);
            close(client_fd);
            continue;
        }
        *pclient = client_fd;
        if (pthread_create(&tid, NULL, client_thread, pclient) != 0)
        {
            perror("pthread_create");
            free(pclient);
            remove_client(client_fd);
            close(client_fd);
            continue;
        }
    }

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i] != 0)
        {
            shutdown(clients[i], SHUT_RDWR);
            close(clients[i]);
            clients[i] = 0;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    if (server_sock != -1)
        close(server_sock);
    pthread_mutex_destroy(&clients_mutex);

    printf("Server exiting cleanly.\n");
    return 0;
}
