#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define REG_PORT 5000
#define LIST_PORT 6000
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

typedef struct{
    char name[64];
    char ip[INET_ADDRSTRLEN];
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *
handle_registration(void *arg){
    int sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0){
        perror("Registration socket creation failed");
        return NULL;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(REG_PORT);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("Registration bind failed");
        close(sock);
        return NULL;
    }

    printf("Server listening for registrations on port %d\n", REG_PORT);

    while (1){
        memset(buffer, 0, BUFFER_SIZE);
        int n = recvfrom(sock, buffer, BUFFER_SIZE, 0,
                         (struct sockaddr *)&client_addr, &client_len);
        if (n > 0){
            buffer[n] = '\0';
            if (strncmp(buffer, "REG ", 4) == 0){
                char client_name[64];
                sscanf(buffer + 4, "%s", client_name);

                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

                pthread_mutex_lock(&mutex);
                int found = 0;
                for (int i = 0; i < client_count; i++){
                    if (strcmp(clients[i].name, client_name) == 0){
                        strcpy(clients[i].ip, client_ip);
                        found = 1;
                        printf("Updated client: %s from %s\n", client_name, client_ip);
                        break;
                    }
                }

                if (!found && client_count < MAX_CLIENTS){
                    strcpy(clients[client_count].name, client_name);
                    strcpy(clients[client_count].ip, client_ip);
                    client_count++;
                    printf("Registered new client: %s from %s\n", client_name, client_ip);
                }

                pthread_mutex_unlock(&mutex);
            }
        }
    }

    close(sock);
    return NULL;
}

void *handle_list_request(void *arg){
    int sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0){
        perror("List socket creation failed");
        return NULL;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(LIST_PORT);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("List bind failed");
        close(sock);
        return NULL;
    }
    printf("Server listening for LIST requests on port %d\n", LIST_PORT);

    while (1){
        memset(buffer, 0, BUFFER_SIZE);
        int n = recvfrom(sock, buffer, BUFFER_SIZE, 0,
                         (struct sockaddr *)&client_addr, &client_len);
        if (n > 0){
            buffer[n] = '\0';

            if (strcmp(buffer, "LIST") == 0){
                char response[BUFFER_SIZE];

                pthread_mutex_lock(&mutex);

                sprintf(response, "LIST %d", client_count);
                for (int i = 0; i < client_count; i++){
                    char entry[128];
                    sprintf(entry, " %s %s", clients[i].name, clients[i].ip);
                    strcat(response, entry);
                }

                pthread_mutex_unlock(&mutex);

                sendto(sock, response, strlen(response), 0,
                       (struct sockaddr *)&client_addr, client_len);

                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
                printf("Sent LIST to %s\n", client_ip);
            }
        }
    }

    close(sock);
    return NULL;
}

int main(){
    pthread_t reg_thread, list_thread;
    printf("=== UDP File Transfer Server ===\n");

    if (pthread_create(&reg_thread, NULL, handle_registration, NULL) != 0){
        perror("Failed to create registration thread");
        exit(1);
    }

    if (pthread_create(&list_thread, NULL, handle_list_request, NULL) != 0){
        perror("Failed to create list thread");
        exit(1);
    }
    pthread_join(reg_thread, NULL);
    pthread_join(list_thread, NULL);

    return 0;
}
