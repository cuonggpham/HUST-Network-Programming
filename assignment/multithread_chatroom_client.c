#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define PORT 8888

int client_socket;
int running = 1;

// Thread nhận tin nhắn từ server
void *receive_messages(void *arg) {
  char buffer[BUFFER_SIZE];
  int recv_len;

  while (running) {
    memset(buffer, 0, BUFFER_SIZE);
    recv_len = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

    if (recv_len <= 0) {
      printf("\nDisconnected from server\n");
      running = 0;
      break;
    }

    buffer[recv_len] = '\0';
    printf("%s", buffer);
    fflush(stdout);
  }

  return NULL;
}

int main(int argc, char *argv[]) {
  struct sockaddr_in server_addr;
  pthread_t recv_thread;
  char buffer[BUFFER_SIZE];
  char *server_ip = "127.0.0.1";

  // Kiểm tra tham số dòng lệnh
  if (argc > 1) {
    server_ip = argv[1];
  }

  // Tạo socket
  client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket < 0) {
    perror("Socket creation failed");
    exit(1);
  }

  // Cấu hình địa chỉ server
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);

  if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
    perror("Invalid address");
    exit(1);
  }

  // Kết nối đến server
  if (connect(client_socket, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) < 0) {
    perror("Connection failed");
    exit(1);
  }

  printf("Connected to chat room server at %s:%d\n", server_ip, PORT);
  printf("Type your messages and press Enter to send\n");
  printf("Press Ctrl+C to exit\n\n");

  // Tạo thread nhận tin nhắn
  if (pthread_create(&recv_thread, NULL, receive_messages, NULL) != 0) {
    perror("Thread creation failed");
    exit(1);
  }

  // Thread chính gửi tin nhắn
  while (running) {
    memset(buffer, 0, BUFFER_SIZE);

    if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
      break;
    }

    if (!running) {
      break;
    }

    // Gửi tin nhắn đến server
    if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
      perror("Send failed");
      break;
    }
  }

  // Đóng kết nối
  running = 0;
  close(client_socket);
  pthread_cancel(recv_thread);
  pthread_join(recv_thread, NULL);

  printf("Disconnected from chat room\n");

  return 0;
}
