/*
 * CHUONG TRINH: TCP Client - Giai quyet ten mien va ket noi toi server
 *
 * KIEN THUC LAP TRINH MANG:
 * - getaddrinfo(): Giai quyet ten mien (DNS resolution) thanh dia chi IP
 * - socket(): Tao socket TCP
 * - connect(): Ket noi toi server
 * - send()/recv(): Gui va nhan du lieu qua TCP
 * - close(): Dong ket noi socket
 */

#include <arpa/inet.h>
#include <malloc.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
  {
    char *domain = "vnexpress.net";
    char *service = "http";

    // struct addrinfo: Cau truc chua thong tin dia chi mang
    // Duoc dinh nghia trong <netdb.h>, dung cho DNS resolution
    struct addrinfo *result = NULL;

    // getaddrinfo(): Giai quyet ten mien thanh danh sach dia chi IP
    // Tra ve 0 neu thanh cong, error code neu that bai
    // result la output parameter - con tro toi linked list cac dia chi
    int error = getaddrinfo(domain, service, NULL, &result);
    if (error == 0 && result != NULL) {
      // Duyet qua linked list de tim dia chi IPv4
      // Linked list: moi node co con tro ai_next tro toi node tiep theo
      struct addrinfo *tmp = result;

      // struct sockaddr: Generic socket address (14 bytes data)
      // struct sockaddr_in: IPv4 specific address (co sin_addr, sin_port)
      struct sockaddr ipv4_sockaddr;
      struct sockaddr_in ipv4_sockaddr_in;

      while (tmp != NULL) {
        // ai_family: Address family - AF_INET (IPv4) hoac AF_INET6 (IPv6)
        if (tmp->ai_family == AF_INET) {
          // memcpy(): Sao chep vung nho (memory copy)
          // memcpy(dest, src, size): Copy 'size' bytes tu 'src' sang 'dest'
          // Dung khi copy struct hoac binary data
          memcpy(&ipv4_sockaddr, tmp->ai_addr, tmp->ai_addrlen);
          memcpy(&ipv4_sockaddr_in, tmp->ai_addr, tmp->ai_addrlen);

          // inet_ntoa(): Network to ASCII - chuyen struct in_addr thanh chuoi
          // "x.x.x.x" CHU Y: Tra ve static buffer, khong thread-safe
          char *ipv4_str = inet_ntoa(ipv4_sockaddr_in.sin_addr);

          // ntohs(): Network to Host Short
          // Chuyen 16-bit tu Network Byte Order (Big Endian) sang Host Byte
          // Order Can thiet vi network luon dung Big Endian, nhung CPU co the
          // dung Little Endian
          printf("%s:%d\n", ipv4_str, ntohs(ipv4_sockaddr_in.sin_port));
          break;
        }
        tmp = tmp->ai_next; // Di chuyen toi node tiep theo trong linked list
      }

      // freeaddrinfo(): Giai phong bo nho da cap phat boi getaddrinfo()
      // QUAN TRONG: Luon goi sau khi dung xong result
      freeaddrinfo(result);

      if (tmp != NULL) {
        // socket(): Tao socket moi
        // AF_INET: IPv4, SOCK_STREAM: TCP (connection-oriented), IPPROTO_TCP:
        // TCP protocol Tra ve file descriptor (so nguyen duong), -1 neu loi
        int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        // connect(): Ket noi toi server (TCP 3-way handshake)
        // Tra ve 0 neu thanh cong, -1 neu that bai
        error = connect(s, &ipv4_sockaddr, sizeof(struct sockaddr));
        if (error == 0) {
          printf("Connected!");
          char *welcom = "HELLO\r\n\r\n";

          // send(): Gui du lieu qua TCP socket
          // send(socket, buffer, length, flags)
          // flags = 0: Khong co flag dac biet
          // Tra ve so bytes gui thanh cong, -1 neu loi
          send(s, welcom, strlen(welcom), 0);

          char buffer[1000] = {0};

          // recv(): Nhan du lieu tu TCP socket (blocking call)
          // Tra ve so bytes nhan duoc, 0 neu connection dong, -1 neu loi
          recv(s, buffer, sizeof(buffer) - 1, 0);

          // close(): Dong socket, giai phong file descriptor
          close(s);
        } else {
          printf("Failed to connect!\n");
        }
      }
    }
  }
}