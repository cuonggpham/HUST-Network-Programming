/*
 * CHUONG TRINH: TFTP Server - Trivial File Transfer Protocol
 *
 * KIEN THUC LAP TRINH MANG:
 * - TFTP (RFC 1350): Giao thuc truyen file don gian tren UDP
 * - RRQ (Read Request): Yeu cau doc file tu server
 * - WRQ (Write Request): Yeu cau ghi file len server
 * - DATA packet: Chua du lieu file (toi da 512 bytes moi packet)
 * - ACK packet: Xac nhan da nhan DATA
 * - Block number: Danh so thu tu cac block du lieu (tu 1)
 */

#include <arpa/inet.h>
#include <errno.h>
#include <malloc.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

// Dinh nghia TFTP Operation codes theo RFC 1350
// const: Bien khong doi gia tri (read-only)
// short: 16-bit integer (TFTP opcode la 2 bytes)
const short RRQ = 1;  // Read Request
const short WRQ = 2;  // Write Request
const short DATA = 3; // Data packet
const short ACK = 4;  // Acknowledgment
const short ERR = 5;  // Error

// Struct chua thong tin truyen vao thread
// typedef struct: Dinh nghia struct va dat ten cung luc
typedef struct _arg {
  SOCKADDR_IN addr;  // Dia chi client
  char buffer[1024]; // Du lieu request
} ARG;

void *ReadThread(void *arg) {
  // Type casting void* ve ARG*
  ARG *tmp = (ARG *)arg;
  char buffer[1024] = {0};
  memcpy(buffer, tmp->buffer, sizeof(buffer));

  // Ten file bat dau tu byte thu 2 (sau opcode 2 bytes)
  // buffer[0-1] = opcode, buffer[2...] = filename + '\0' + mode + '\0'
  char file[1024] = {0};
  strcpy(file, buffer + 2);

  int d = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  // struct timeval: Cau truc luu thoi gian (seconds + microseconds)
  // Dinh nghia trong <sys/time.h>
  struct timeval to;
  to.tv_sec = 5;  // 5 giay
  to.tv_usec = 0; // 0 microseconds

  // setsockopt() voi SO_RCVTIMEO: Dat timeout cho recv()
  // Neu khong nhan data trong 5 giay, recv() tra ve -1 voi errno = EAGAIN
  setsockopt(d, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to));

  FILE *f = fopen(file, "rb");
  if (f != NULL) {
    // unsigned short: 16-bit khong dau (0 - 65535)
    // Block number trong TFTP bat dau tu 1
    unsigned short block = 1;
    int r = 0;

    while (!feof(f)) {
      // TFTP DATA Packet structure (RFC 1350):
      // [Opcode: 2 bytes][Block#: 2 bytes][Data: 0-512 bytes]
      char packet[1024] = {0};

      // Doc du lieu vao vi tri byte thu 4 (sau header)
      r = fread(packet + 4, 1, 512, f);

      // Tao header: Opcode = DATA (2 bytes, big-endian)
      packet[0] = 0;    // Byte cao cua opcode (DATA = 0x0003)
      packet[1] = DATA; // Byte thap cua opcode

      // Block number (2 bytes, big-endian / network byte order)
      // Bitwise operations: Tach so 16-bit thanh 2 bytes
      // (block >> 8): Dich phai 8 bit -> lay byte cao
      // & 0xFF: Mask de lay 8 bit thap nhat
      packet[2] = (block >> 8) & 0xFF; // Byte cao (MSB)
      packet[3] = block & 0xFF;        // Byte thap (LSB)

      unsigned short opcode = 0;
      // Retransmission: Gui lai cho den khi nhan ACK
      while (opcode != ACK) {
        // Gui packet: header (4 bytes) + data (r bytes)
        sendto(d, packet, r + 4, 0, (SOCKADDR *)&tmp->addr, sizeof(SOCKADDR));

        char ack[1024] = {0};
        int r = recvfrom(d, ack, sizeof(ack), 0, NULL, 0);

        // Parse opcode tu ACK packet (2 bytes, big-endian)
        // (ack[0] << 8): Dich trai byte cao 8 bit
        // | ack[1]: OR voi byte thap
        opcode = (ack[0] << 8) | ack[1];
      }
      block++;
    }

    // Neu block cuoi cung day du 512 bytes, phai gui them 1 packet trong
    // de bao hieu ket thuc (theo TFTP protocol)
    if (r == 512) {
      char packet[1024] = {0};
      packet[0] = 0;
      packet[1] = DATA;
      packet[2] = (block >> 8) & 0xFF;
      packet[3] = block & 0xFF;
      // Packet chi co header, khong co data (length = 4)
      sendto(d, packet, 4, 0, (SOCKADDR *)&tmp->addr, sizeof(SOCKADDR));
    }

    fclose(f);
    close(d);
    free(arg);
  } else {
    // Gui ERROR packet neu khong mo duoc file
    // TFTP ERROR: [Opcode=5][ErrorCode][ErrMsg]['\0']
    char packet[1024] = {0};
    packet[0] = 0;
    packet[1] = ERR;
    packet[2] = 0;
    packet[3] = 0;
    strcpy(packet + 4, "Failed to open");
    // strlen + 5: 4 bytes header + 1 byte null terminator
    sendto(d, packet, strlen(packet + 4) + 5, 0, (SOCKADDR *)&tmp->addr,
           sizeof(SOCKADDR));
    close(d);
    free(arg);
  }

  return NULL;
}

void *WriteThread(void *arg) {
  // Chua implement
  return NULL;
}

int main() {
  int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  SOCKADDR_IN saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(9999);
  saddr.sin_addr.s_addr = inet_addr("0.0.0.0");

  if (bind(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR)) == 0) {
    char buffer[1024] = {0};
    SOCKADDR_IN caddr;
    int clen = sizeof(SOCKADDR);
    int r = recvfrom(s, buffer, sizeof(buffer), 0, (SOCKADDR *)&caddr, &clen);
    if (r > 0) {
      // Parse opcode tu request packet
      unsigned short opcode = (buffer[0] << 8) | buffer[1];

      if (opcode == RRQ) {
        pthread_t tid;
        // calloc(): Cap phat + khoi tao = 0
        ARG *arg = (ARG *)calloc(1, sizeof(ARG));
        arg->addr = caddr;
        memcpy(arg->buffer, buffer, sizeof(buffer));
        pthread_create(&tid, NULL, ReadThread, arg);
      }
    }
  } else {
    printf("Failed to bind!\n");
  }
  close(s);
  getchar();
}