#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 5000
#define CLIENT_PORT 6000
#define BUFFER_SIZE 1024

int main() {
  int sockfd;
  struct sockaddr_in server_addr, client_addr;
  char buffer[BUFFER_SIZE];
  socklen_t addr_len = sizeof(client_addr);

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(SERVER_PORT);

  if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("Bind failed");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  printf("UDP Server started. Listening on port %d\n", SERVER_PORT);
  printf("Waiting for messages...\n\n");

  while (1) {
    memset(buffer, 0, BUFFER_SIZE);

    int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                     (struct sockaddr *)&client_addr, &addr_len);

    if (n < 0) {
      perror("Receive failed");
      continue;
    }

    buffer[n] = '\0';
    printf("Received from client (%s:%d): %s\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),
           buffer);

    struct sockaddr_in reply_addr;
    memset(&reply_addr, 0, sizeof(reply_addr));
    reply_addr.sin_family = AF_INET;
    reply_addr.sin_addr = client_addr.sin_addr;
    reply_addr.sin_port = htons(CLIENT_PORT);

    if (sendto(sockfd, buffer, strlen(buffer), 0,
               (const struct sockaddr *)&reply_addr, sizeof(reply_addr)) < 0) {
      perror("Send failed");
      continue;
    }

    printf("Echoed back to client: %s\n\n", buffer);
  }

  close(sockfd);
  return 0;
}
