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
  socklen_t addr_len = sizeof(server_addr);

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  memset(&client_addr, 0, sizeof(client_addr));
  client_addr.sin_family = AF_INET;
  client_addr.sin_addr.s_addr = INADDR_ANY;
  client_addr.sin_port = htons(CLIENT_PORT);

  if (bind(sockfd, (const struct sockaddr *)&client_addr, sizeof(client_addr)) <
      0) {
    perror("Bind failed");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  printf("UDP Client started. Listening on port %d\n", CLIENT_PORT);
  printf("Enter text to send (or 'quit' to exit):\n");

  while (1) {
    printf("\n> ");
    fgets(buffer, BUFFER_SIZE, stdin);

    buffer[strcspn(buffer, "\n")] = 0;

    if (strcmp(buffer, "quit") == 0) {
      printf("Exiting...\n");
      break;
    }

    if (sendto(sockfd, buffer, strlen(buffer), 0,
               (const struct sockaddr *)&server_addr,
               sizeof(server_addr)) < 0) {
      perror("Send failed");
      continue;
    }

    printf("Sent to server: %s\n", buffer);

    memset(buffer, 0, BUFFER_SIZE);
    int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                     (struct sockaddr *)&server_addr, &addr_len);

    if (n < 0) {
      perror("Receive failed");
      continue;
    }

    buffer[n] = '\0';
    printf("Received from server: %s\n", buffer);
  }

  close(sockfd);
  return 0;
}
