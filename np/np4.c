#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <malloc.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    //if (argc >= 2)
    {
        char* domain = "vnexpress.net"; //argv[1];
        char* service = "http"; //argv[2];
        struct addrinfo* result = NULL;
        int error = getaddrinfo(domain, service, NULL, &result);
        if (error == 0 && result != NULL)
        {
            struct addrinfo* tmp = result;
            struct sockaddr ipv4_sockaddr;
            struct sockaddr_in ipv4_sockaddr_in;  

            while (tmp != NULL)
            {
                if (tmp->ai_family == AF_INET)
                {
                    memcpy(&ipv4_sockaddr, tmp->ai_addr, tmp->ai_addrlen);        
                    memcpy(&ipv4_sockaddr_in, tmp->ai_addr, tmp->ai_addrlen);
                    char* ipv4_str = inet_ntoa(ipv4_sockaddr_in.sin_addr);
                    printf("%s:%d\n", ipv4_str, ntohs(ipv4_sockaddr_in.sin_port));
                    break;
                }
                tmp = tmp->ai_next;
            }
            freeaddrinfo(result);   

            if (tmp != NULL)
            {
                int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                error = connect(s, &ipv4_sockaddr, sizeof(struct sockaddr));
                if (error == 0)
                {
                    printf("Connected!");
                    char* welcom = "HELLO\r\n\r\n";
                    send(s, welcom, strlen(welcom), 0);
                    char buffer[1000] = { 0 };
                    recv(s, buffer, sizeof(buffer) - 1, 0);
                    close(s);
                }else
                {
                    printf("Failed to connect!\n");
                }
            }
        }
    }
}