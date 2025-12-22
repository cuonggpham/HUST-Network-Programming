#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main() {
  int sockfd;
  struct sockaddr_in server_addr;
  char send_buf[BUFFER_SIZE], recv_buf[BUFFER_SIZE];
  int n;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("socket error");
    exit(1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(5000);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("connect error");
    close(sockfd);
    exit(1);
  }

  printf("Sent (nc -l -p 5000)\n");
  printf("Type content.\n");

  while (1) {
    printf("> ");
    fflush(stdout);
    if (fgets(send_buf, sizeof(send_buf), stdin) == NULL)
      break;

    send(sockfd, send_buf, strlen(send_buf), 0);

    n = recv(sockfd, recv_buf, sizeof(recv_buf) - 1, 0);
    if (n > 0) {
      recv_buf[n] = '\0';
      printf("Server: %s", recv_buf);
    } else if (n == 0) {
      printf("Server closed connection.\n");
      break;
    } else {
      perror("recv error");
      break;
    }
  }

  close(sockfd);
  return 0;
}
