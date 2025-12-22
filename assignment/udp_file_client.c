#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define REG_PORT 5000
#define LIST_PORT 6000
#define SEND_PORT 7000
#define BUFFER_SIZE 1024
#define BROADCAST_IP "255.255.255.255"

char my_name[64];
int tcp_server_sock = -1;
int tcp_server_port = 0;

void broadcast_message(const char *message, int port) {
  int sock;
  struct sockaddr_in broadcast_addr;
  int broadcast_enable = 1;

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    perror("Broadcast socket creation failed");
    return;
  }

  if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable,
                 sizeof(broadcast_enable)) < 0) {
    perror("Broadcast enable failed");
    close(sock);
    return;
  }

  memset(&broadcast_addr, 0, sizeof(broadcast_addr));
  broadcast_addr.sin_family = AF_INET;
  broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);
  broadcast_addr.sin_port = htons(port);

  sendto(sock, message, strlen(message), 0, (struct sockaddr *)&broadcast_addr,
         sizeof(broadcast_addr));

  close(sock);
}

void register_client() {
  srand(time(NULL) + getpid());
  int random_num = rand() % 10000;
  sprintf(my_name, "uname_%d", random_num);

  char reg_message[128];
  sprintf(reg_message, "REG %s", my_name);
  broadcast_message(reg_message, REG_PORT);

  printf("Name: %s\n", my_name);
}

void get_client_list() {
  int sock;
  struct sockaddr_in server_addr, recv_addr, bind_addr;
  socklen_t addr_len = sizeof(recv_addr);
  char buffer[BUFFER_SIZE];
  struct timeval tv;

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    perror("List socket creation failed");
    return;
  }

  int broadcast_enable = 1;
  setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable,
             sizeof(broadcast_enable));

  memset(&bind_addr, 0, sizeof(bind_addr));
  bind_addr.sin_family = AF_INET;
  bind_addr.sin_addr.s_addr = INADDR_ANY;
  bind_addr.sin_port = 0;

  if (bind(sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
    perror("List bind failed");
    close(sock);
    return;
  }

  tv.tv_sec = 3;
  tv.tv_usec = 0;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  struct sockaddr_in broadcast_addr;
  memset(&broadcast_addr, 0, sizeof(broadcast_addr));
  broadcast_addr.sin_family = AF_INET;
  broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);
  broadcast_addr.sin_port = htons(LIST_PORT);

  const char *message = "LIST";
  sendto(sock, message, strlen(message), 0, (struct sockaddr *)&broadcast_addr,
         sizeof(broadcast_addr));

  memset(buffer, 0, BUFFER_SIZE);
  int n = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&recv_addr,
                   &addr_len);

  if (n > 0) {
    buffer[n] = '\0';

    int count;
    char *ptr = buffer;

    if (sscanf(ptr, "LIST %d", &count) == 1) {
      printf("\n=== %d Client(s) ===\n", count);

      ptr = strchr(ptr, ' ');
      if (ptr)
        ptr++;
      ptr = strchr(ptr, ' ');
      if (ptr)
        ptr++;

      for (int i = 0; i < count; i++) {
        char name[64], ip[INET_ADDRSTRLEN];
        if (sscanf(ptr, "%s %s", name, ip) == 2) {
          printf("%d. %s\t%s\n", i + 1, name, ip);

          ptr = strchr(ptr, ' ');
          if (ptr)
            ptr++;
          ptr = strchr(ptr, ' ');
          if (ptr)
            ptr++;
        }
      }
      printf("================\n\n");
    }
  } else {
    printf("No response (timeout)\n");
  }

  close(sock);
}

int send_file_tcp(const char *filename, const char *ip, int port) {
  int sock;
  struct sockaddr_in server_addr;
  FILE *file;
  char buffer[BUFFER_SIZE];

  file = fopen(filename, "rb");
  if (!file) {
    printf("Error: Cannot open file %s\n", filename);
    return -1;
  }

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("TCP socket creation failed");
    fclose(file);
    return -1;
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(ip);
  server_addr.sin_port = htons(port);

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Connect failed");
    close(sock);
    fclose(file);
    return -1;
  }

  printf("Connected to %s:%d\n", ip, port);

  uint32_t size_network = htonl((uint32_t)file_size);
  send(sock, &size_network, sizeof(size_network), 0);

  long sent = 0;
  size_t bytes_read;
  while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
    send(sock, buffer, bytes_read, 0);
    sent += bytes_read;
  }

  printf("Sent %ld bytes\n", sent);

  close(sock);
  fclose(file);
  return 0;
}

void *receive_send_request(void *arg) {
  int sock;
  struct sockaddr_in recv_addr, sender_addr;
  socklen_t sender_len = sizeof(sender_addr);
  char buffer[BUFFER_SIZE];

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    perror("Receive socket creation failed");
    return NULL;
  }

  int reuse = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    perror("SO_REUSEADDR failed");
  }
#ifdef SO_REUSEPORT
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
    perror("SO_REUSEPORT failed");
  }
#endif

  memset(&recv_addr, 0, sizeof(recv_addr));
  recv_addr.sin_family = AF_INET;
  recv_addr.sin_addr.s_addr = INADDR_ANY;
  recv_addr.sin_port = htons(SEND_PORT);

  if (bind(sock, (struct sockaddr *)&recv_addr, sizeof(recv_addr)) < 0) {
    perror("Receive bind failed");
    close(sock);
    return NULL;
  }

  printf("Listening on port %d\n", SEND_PORT);

  while (1) {
    memset(buffer, 0, BUFFER_SIZE);
    int n = recvfrom(sock, buffer, BUFFER_SIZE, 0,
                     (struct sockaddr *)&sender_addr, &sender_len);

    if (n > 0) {
      buffer[n] = '\0';

      if (strncmp(buffer, "SEND ", 5) == 0) {
        char filename[256], target_name[64], sender_name[64];
        int sender_port;
        if (sscanf(buffer + 5, "%s %s %s %d", filename, target_name,
                   sender_name, &sender_port) == 4) {
          if (strcmp(target_name, my_name) == 0) {
            printf("\n[Incoming: %s from %s]\n", filename, sender_name);

            int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
            if (tcp_sock < 0) {
              perror("TCP server socket creation failed");
              continue;
            }

            struct sockaddr_in tcp_addr;
            memset(&tcp_addr, 0, sizeof(tcp_addr));
            tcp_addr.sin_family = AF_INET;
            tcp_addr.sin_addr.s_addr = INADDR_ANY;
            tcp_addr.sin_port = 0;

            if (bind(tcp_sock, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr)) <
                0) {
              perror("TCP bind failed");
              close(tcp_sock);
              continue;
            }

            listen(tcp_sock, 1);

            socklen_t tcp_len = sizeof(tcp_addr);
            getsockname(tcp_sock, (struct sockaddr *)&tcp_addr, &tcp_len);
            int my_port = ntohs(tcp_addr.sin_port);

            char my_ip[INET_ADDRSTRLEN] = "0.0.0.0";
            char response[128];
            sprintf(response, "%s %d", my_ip, my_port);

            struct sockaddr_in response_addr;
            memset(&response_addr, 0, sizeof(response_addr));
            response_addr.sin_family = AF_INET;
            response_addr.sin_addr = sender_addr.sin_addr;
            response_addr.sin_port = htons(sender_port);

            sendto(sock, response, strlen(response), 0,
                   (struct sockaddr *)&response_addr, sizeof(response_addr));

            printf("Waiting on port %d...\n", my_port);

            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_sock =
                accept(tcp_sock, (struct sockaddr *)&client_addr, &client_len);

            if (client_sock >= 0) {
              uint32_t file_size_network;
              recv(client_sock, &file_size_network, sizeof(file_size_network),
                   0);
              uint32_t file_size = ntohl(file_size_network);

              printf("Receiving: %s (%u bytes)\n", filename, file_size);

              char save_filename[300];
              sprintf(save_filename, "received_%s", filename);
              FILE *file = fopen(save_filename, "wb");

              if (file) {
                char file_buffer[BUFFER_SIZE];
                uint32_t received = 0;

                while (received < file_size) {
                  int bytes = recv(client_sock, file_buffer, BUFFER_SIZE, 0);
                  if (bytes <= 0)
                    break;

                  fwrite(file_buffer, 1, bytes, file);
                  received += bytes;
                }

                fclose(file);
                printf("Saved: %s (%u bytes)\n\n", save_filename, received);
              }

              close(client_sock);
            }

            close(tcp_sock);
          }
        }
      }
    }
  }

  close(sock);
  return NULL;
}

void send_file_to_client(const char *filename, const char *target_name) {
  int sock;
  struct sockaddr_in recv_addr;
  socklen_t addr_len = sizeof(recv_addr);
  char buffer[BUFFER_SIZE];
  struct timeval tv;

  FILE *test_file = fopen(filename, "rb");
  if (!test_file) {
    printf("Error: File %s not found\n", filename);
    return;
  }
  fclose(test_file);

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    perror("Send socket creation failed");
    return;
  }

  struct sockaddr_in bind_addr;
  memset(&bind_addr, 0, sizeof(bind_addr));
  bind_addr.sin_family = AF_INET;
  bind_addr.sin_addr.s_addr = INADDR_ANY;
  bind_addr.sin_port = 0;

  if (bind(sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
    perror("Bind failed");
    close(sock);
    return;
  }

  socklen_t bind_len = sizeof(bind_addr);
  getsockname(sock, (struct sockaddr *)&bind_addr, &bind_len);
  int my_listen_port = ntohs(bind_addr.sin_port);

  tv.tv_sec = 5;
  tv.tv_usec = 0;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  char send_message[512];
  sprintf(send_message, "SEND %s %s %s %d", filename, target_name, my_name,
          my_listen_port);
  broadcast_message(send_message, SEND_PORT);

  printf("Waiting for %s...\n", target_name);

  memset(buffer, 0, BUFFER_SIZE);
  int n = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&recv_addr,
                   &addr_len);

  if (n > 0) {
    buffer[n] = '\0';

    char ip[INET_ADDRSTRLEN];
    int port;

    if (sscanf(buffer, "%s %d", ip, &port) == 2) {
      if (strcmp(ip, "0.0.0.0") == 0) {
        inet_ntop(AF_INET, &recv_addr.sin_addr, ip, INET_ADDRSTRLEN);
      }

      printf("Response: %s:%d\n", ip, port);

      if (send_file_tcp(filename, ip, port) == 0) {
        printf("Transfer complete!\n");
      } else {
        printf("Transfer failed!\n");
      }
    }
  } else {
    printf("No response (timeout)\n");
  }

  close(sock);
}

int main() {
  pthread_t receive_thread;
  char command[512];

  printf("=== UDP File Transfer Client ===\n");

  register_client();
  sleep(1);

  if (pthread_create(&receive_thread, NULL, receive_send_request, NULL) != 0) {
    perror("Failed to create receive thread");
    exit(1);
  }

  printf("\nCommands: LIST | SEND <file> <name> | QUIT\n\n");

  while (1) {
    printf("> ");
    fflush(stdout);

    if (fgets(command, sizeof(command), stdin) == NULL) {
      break;
    }

    command[strcspn(command, "\n")] = 0;

    if (strlen(command) == 0) {
      continue;
    }

    if (strcmp(command, "LIST") == 0) {
      get_client_list();
    } else if (strncmp(command, "SEND ", 5) == 0) {
      char filename[256], target_name[64];
      if (sscanf(command + 5, "%s %s", filename, target_name) == 2) {
        send_file_to_client(filename, target_name);
      } else {
        printf("Usage: SEND <file> <name>\n");
      }
    } else if (strcmp(command, "QUIT") == 0) {
      break;
    } else {
      printf("Unknown command\n");
    }
  }
  return 0;
}
