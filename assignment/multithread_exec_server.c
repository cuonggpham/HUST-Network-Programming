#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <errno.h>
#include <pthread.h>

#define PORT 9999
#define MAX_CLIENTS 1024
#define CMD_BUF_SIZE 1024
#define RESULT_BUF_SIZE 4096
#define OUTPUT_FILE "tmp.txt"

int clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

int server_sock = -1;
volatile sig_atomic_t stop_server = 0;

void handle_sigint(int sig) {
  (void)sig;
  stop_server = 1;
  if (server_sock != -1) {
    shutdown(server_sock, SHUT_RDWR);
    close(server_sock);
    server_sock = -1;
  }
}

int add_client(int sockfd) {
  pthread_mutex_lock(&clients_mutex);
  if (client_count >= MAX_CLIENTS) {
    pthread_mutex_unlock(&clients_mutex);
    return -1;
  }
  for (int i = 0; i < MAX_CLIENTS; ++i) {
    if (clients[i] == 0) {
      clients[i] = sockfd;
      client_count++;
      pthread_mutex_unlock(&clients_mutex);
      return 0;
    }
  }
  pthread_mutex_unlock(&clients_mutex);
  return -1;
}

void remove_client(int sockfd) {
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; ++i) {
    if (clients[i] == sockfd) {
      clients[i] = 0;
      client_count--;
      break;
    }
  }
  pthread_mutex_unlock(&clients_mutex);
}

void write_to_file(int client_sock, const char *client_addr, const char *cmd,
                   const char *result) {
  pthread_mutex_lock(&file_mutex);

  FILE *fp = fopen(OUTPUT_FILE, "a");
  if (fp) {
    time_t now = time(NULL);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(fp, "=================================\n");
    fprintf(fp, "Time: %s\n", time_buf);
    fprintf(fp, "Client: %s (socket %d)\n", client_addr, client_sock);
    fprintf(fp, "Command: %s\n", cmd);
    fprintf(fp, "Output:\n%s", result);
    if (result[0] != '\0' && result[strlen(result) - 1] != '\n')
      fprintf(fp, "\n");
    fprintf(fp, "=================================\n\n");
    fclose(fp);
  } else {
    perror("fopen");
  }

  pthread_mutex_unlock(&file_mutex);
}

void *client_thread(void *arg) {
  int client_sock = *((int *)arg);
  char *client_addr = strdup((char *)arg + sizeof(int));
  free(arg);

  pthread_detach(pthread_self());

  printf("Thread %lu: handling client %s (socket %d)\n",
         (unsigned long)pthread_self(), client_addr, client_sock);

  char cmd[CMD_BUF_SIZE];
  char result[RESULT_BUF_SIZE];

  const char *welcome =
      "Multi-threaded Command Execution Server\nType 'exit' to disconnect.\n";
  send(client_sock, welcome, strlen(welcome), 0);

  while (1) {
    memset(cmd, 0, sizeof(cmd));
    ssize_t r = recv(client_sock, cmd, sizeof(cmd) - 1, 0);

    if (r <= 0) {
      if (r == 0)
        printf("Thread %lu: client %s disconnected\n",
               (unsigned long)pthread_self(), client_addr);
      else if (errno != EINTR)
        printf("Thread %lu: recv error from %s: %s\n",
               (unsigned long)pthread_self(), client_addr, strerror(errno));
      break;
    }

    cmd[strcspn(cmd, "\r\n")] = 0;

    if (strlen(cmd) == 0)
      continue;

    printf("Thread %lu: client %s executing: %s\n",
           (unsigned long)pthread_self(), client_addr, cmd);

    if (strcmp(cmd, "exit") == 0) {
      printf("Thread %lu: client %s requested exit\n",
             (unsigned long)pthread_self(), client_addr);
      const char *msg = "Goodbye!\n";
      send(client_sock, msg, strlen(msg), 0);
      break;
    }

    FILE *fp = popen(cmd, "r");
    if (!fp) {
      const char *err = "Failed to execute command.\n";
      send(client_sock, err, strlen(err), 0);

      write_to_file(client_sock, client_addr, cmd,
                    "ERROR: Failed to execute command\n");
      continue;
    }

    memset(result, 0, sizeof(result));
    size_t bytes_read = fread(result, 1, sizeof(result) - 1, fp);
    int exit_status = pclose(fp);

    if (exit_status != 0 && bytes_read < sizeof(result) - 100) {
      char status_msg[100];
      snprintf(status_msg, sizeof(status_msg),
               "\n[Command exited with status %d]\n", WEXITSTATUS(exit_status));
      strncat(result, status_msg, sizeof(result) - strlen(result) - 1);
    }

    if (strlen(result) > 0) {
      send(client_sock, result, strlen(result), 0);
    } else {
      const char *msg = "[Command executed successfully with no output]\n";
      send(client_sock, msg, strlen(msg), 0);
      strcpy(result, msg);
    }

    write_to_file(client_sock, client_addr, cmd, result);
  }

  remove_client(client_sock);
  close(client_sock);
  free(client_addr);

  printf("Thread %lu: exiting\n", (unsigned long)pthread_self());
  return NULL;
}

int main() {
  for (int i = 0; i < MAX_CLIENTS; ++i)
    clients[i] = 0;

  struct sigaction sa;
  sa.sa_handler = handle_sigint;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);

  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock < 0) {
    perror("socket");
    return 1;
  }

  int opt = 1;
  if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    perror("setsockopt");
    close(server_sock);
    return 1;
  }

  struct sockaddr_in saddr;
  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(PORT);
  saddr.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
    perror("bind");
    close(server_sock);
    return 1;
  }

  if (listen(server_sock, 10) < 0) {
    perror("listen");
    close(server_sock);
    return 1;
  }

  printf("Multi-threaded Command Execution Server started on port %d\n", PORT);
  printf("All command outputs will be logged to: %s\n", OUTPUT_FILE);

  FILE *fp = fopen(OUTPUT_FILE, "w");
  if (fp) {
    fprintf(fp, "=== Command Execution Server Log ===\n");
    fprintf(fp, "Server started at: ");
    time_t now = time(NULL);
    fprintf(fp, "%s\n\n", ctime(&now));
    fclose(fp);
  }

  while (!stop_server) {
    struct sockaddr_in caddr;
    socklen_t clen = sizeof(caddr);

    printf("Main thread: waiting for connection...\n");
    int client_fd = accept(server_sock, (struct sockaddr *)&caddr, &clen);

    if (client_fd < 0) {
      if (stop_server)
        break;
      perror("accept");
      continue;
    }

    char client_addr_str[INET_ADDRSTRLEN + 16];
    snprintf(client_addr_str, sizeof(client_addr_str), "%s:%d",
             inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));

    printf("Main thread: client connected from %s (socket %d)\n",
           client_addr_str, client_fd);

    if (add_client(client_fd) != 0) {
      const char *msg =
          "Server is full. Maximum clients reached. Try again later.\n";
      send(client_fd, msg, strlen(msg), 0);
      close(client_fd);
      printf("Main thread: rejected client (server full)\n");
      continue;
    }

    size_t arg_size = sizeof(int) + strlen(client_addr_str) + 1;
    char *thread_arg = malloc(arg_size);
    if (!thread_arg) {
      perror("malloc");
      remove_client(client_fd);
      close(client_fd);
      continue;
    }

    memcpy(thread_arg, &client_fd, sizeof(int));
    strcpy(thread_arg + sizeof(int), client_addr_str);

    pthread_t tid;
    if (pthread_create(&tid, NULL, client_thread, thread_arg) != 0) {
      perror("pthread_create");
      free(thread_arg);
      remove_client(client_fd);
      close(client_fd);
      continue;
    }
  }

  printf("\nShutting down server...\n");
  pthread_mutex_lock(&clients_mutex);
  for (int i = 0; i < MAX_CLIENTS; ++i) {
    if (clients[i] != 0) {
      shutdown(clients[i], SHUT_RDWR);
      close(clients[i]);
      clients[i] = 0;
    }
  }
  pthread_mutex_unlock(&clients_mutex);

  if (server_sock != -1)
    close(server_sock);

  pthread_mutex_destroy(&clients_mutex);
  pthread_mutex_destroy(&file_mutex);

  printf("Server exited cleanly.\n");
  return 0;
}
