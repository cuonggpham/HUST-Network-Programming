#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main() {
  int server, client;
  struct sockaddr_in addr;
  char buffer[1024];

  server = socket(AF_INET, SOCK_STREAM, 0);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  / addr.sin_port = htons(8888);

  bind(server, (struct sockaddr *)&addr, sizeof(addr));
  listen(server, 5);

  printf("Server listening on port 8888...\n");

  while (1) {
    client = accept(server, NULL, NULL);
    printf("Client connected!\n");

    send(client, "Welcome to my  server!\n", 33, 0);

    memset(buffer, 0, sizeof(buffer));
    recv(client, buffer, sizeof(buffer) - 1, 0);
    printf("Client says: %s\n", buffer);

    close(client);
    printf("Connection closed.\n");
  }

  close(server);
  return 0;
}
