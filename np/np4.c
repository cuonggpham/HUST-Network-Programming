/*
 * CHUONG TRINH: TCP Client - Giai quyet ten mien va ket noi toi server
 *
 * KIEN THUC LAP TRINH MANG:
 * - getaddrinfo(): Giai quyet ten mien (DNS resolution) thanh dia chi IP
 * - socket(): Tao socket TCP
 * - connect(): Ket noi toi server
 * - send()/recv(): Gui va nhan du lieu qua TCP
 * - close(): Dong ket noi socket
 *
 * KIEN THUC C ADVANCE:
 * - struct addrinfo: Cau truc chua thong tin dia chi mang
 * - struct sockaddr_in: Cau truc dia chi IPv4
 * - memcpy(): Sao chep vung nho
 * - inet_ntoa(): Chuyen struct in_addr thanh chuoi IP
 * - ntohs(): Network to Host Short - Chuyen doi byte order
 * - Linked list: addrinfo->ai_next la danh sach lien ket
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <malloc.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    // if (argc >= 2)
    {
        char *domain = "vnexpress.net"; // argv[1];
        char *service = "http";         // argv[2];
        struct addrinfo *result = NULL;

        // getaddrinfo() giai quyet ten mien thanh dia chi IP
        // Tra ve 0 neu thanh cong, gia tri khac 0 neu loi
        // result se chua danh sach lien ket cac dia chi IP
        int error = getaddrinfo(domain, service, NULL, &result);
        if (error == 0 && result != NULL)
        {
            struct addrinfo *tmp = result;
            struct sockaddr ipv4_sockaddr;
            struct sockaddr_in ipv4_sockaddr_in;

            // Duyet linked list de tim dia chi IPv4
            // ai_next tro toi node tiep theo trong danh sach
            while (tmp != NULL)
            {
                // AF_INET la constant dai dien cho IPv4
                // AF_INET6 dai dien cho IPv6
                if (tmp->ai_family == AF_INET)
                {
                    // memcpy() sao chep du lieu tu nguon sang dich
                    // memcpy(destination, source, size)
                    memcpy(&ipv4_sockaddr, tmp->ai_addr, tmp->ai_addrlen);
                    memcpy(&ipv4_sockaddr_in, tmp->ai_addr, tmp->ai_addrlen);

                    // inet_ntoa() chuyen struct in_addr thanh chuoi IP dang "x.x.x.x"
                    char *ipv4_str = inet_ntoa(ipv4_sockaddr_in.sin_addr);

                    // ntohs() chuyen Network Byte Order sang Host Byte Order
                    // Mang dung Big Endian, may tinh co the dung Little Endian
                    printf("%s:%d\n", ipv4_str, ntohs(ipv4_sockaddr_in.sin_port));
                    break;
                }
                tmp = tmp->ai_next;
            }

            // freeaddrinfo() giai phong bo nho da cap phat boi getaddrinfo()
            freeaddrinfo(result);

            if (tmp != NULL)
            {
                // socket() tao socket moi
                // AF_INET: IPv4, SOCK_STREAM: TCP, IPPROTO_TCP: giao thuc TCP
                int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

                // connect() ket noi toi server
                // Tra ve 0 neu thanh cong, -1 neu that bai
                error = connect(s, &ipv4_sockaddr, sizeof(struct sockaddr));
                if (error == 0)
                {
                    printf("Connected!");
                    char *welcom = "HELLO\r\n\r\n";

                    // send() gui du lieu qua socket
                    // Tham so 4 la flags (0 = khong co flag dac biet)
                    send(s, welcom, strlen(welcom), 0);

                    char buffer[1000] = {0};

                    // recv() nhan du lieu tu socket
                    // Tra ve so byte nhan duoc, 0 neu dong ket noi, -1 neu loi
                    recv(s, buffer, sizeof(buffer) - 1, 0);

                    // close() dong socket va giai phong tai nguyen
                    close(s);
                }
                else
                {
                    printf("Failed to connect!\n");
                }
            }
        }
    }
}