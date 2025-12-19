/*
 * CHUONG TRINH: TFTP Server - Trivial File Transfer Protocol
 *
 * KIEN THUC LAP TRINH MANG:
 * - TFTP: Giao thuc truyen file don gian tren UDP
 * - RRQ (Read Request): Yeu cau doc file
 * - WRQ (Write Request): Yeu cau ghi file
 * - DATA packet: Chua du lieu file (toi da 512 bytes)
 * - ACK packet: Xac nhan da nhan DATA
 * - Block number: Danh so thu tu cac block du lieu
 * - Timeout va retransmission: Gui lai neu khong nhan ACK
 *
 * KIEN THUC C ADVANCE:
 * - setsockopt(): SO_RCVTIMEO de set timeout cho recv()
 * - struct timeval: Cau truc thoi gian cho timeout
 * - Bitwise operations: (block >> 8) & 0xFF de tach byte cao/thap
 * - Network byte order: TFTP dung big-endian
 */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

// TFTP Operation codes
const short RRQ = 1;  // Read Request
const short WRQ = 2;  // Write Request
const short DATA = 3; // Data packet
const short ACK = 4;  // Acknowledgment
const short ERR = 5;  // Error

typedef struct _arg
{
    SOCKADDR_IN addr;
    char buffer[1024];
} ARG;

void *ReadThread(void *arg)
{
    ARG *tmp = (ARG *)arg;
    char buffer[1024] = {0};
    memcpy(buffer, tmp->buffer, sizeof(buffer));
    char file[1024] = {0};
    strcpy(file, buffer + 2);

    int d = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct timeval to;
    to.tv_sec = 5;
    to.tv_usec = 0;
    setsockopt(d, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to));

    FILE *f = fopen(file, "rb");
    if (f != NULL)
    {
        unsigned short block = 1; // Block number bat dau tu 1
        int r = 0;

        // Doc va gui tung block 512 bytes
        while (!feof(f))
        {
            // TFTP Packet structure: [Opcode(2 bytes)][Block#(2 bytes)][Data(0-512 bytes)]
            char packet[1024] = {0};
            r = fread(packet + 4, 1, 512, f); // Doc du lieu vao vi tri thu 4

            // Tao header: Opcode = DATA (2 bytes)
            packet[0] = 0;    // Byte cao cua opcode
            packet[1] = DATA; // Byte thap cua opcode

            // Block number (2 bytes, big-endian)
            packet[2] = (block >> 8) & 0xFF; // Byte cao cua block number
            packet[3] = block & 0xFF;        // Byte thap cua block number

            unsigned short opcode = 0;
            // Gui lai cho den khi nhan duoc ACK
            while (opcode != ACK)
            {
                sendto(d, packet, r + 4, 0, (SOCKADDR *)&tmp->addr, sizeof(SOCKADDR));
                char ack[1024] = {0};
                int r = recvfrom(d, ack, sizeof(ack), 0, NULL, 0);
                opcode = (ack[0] << 8) | ack[1];
            }
            block++;
        }
        if (r == 512)
        {
            char packet[1024] = {0};
            packet[0] = 0;
            packet[1] = DATA;
            packet[2] = (block >> 8) & 0xFF;
            packet[3] = block & 0xFF;
            sendto(d, packet, 4, 0, (SOCKADDR *)&tmp->addr, sizeof(SOCKADDR));
        }

        fclose(f);
        close(d);
        free(arg);
    }
    else
    {
        char packet[1024] = {0};
        packet[0] = 0;
        packet[1] = ERR;
        packet[2] = 0;
        packet[3] = 0;
        strcpy(packet + 4, "Failed to open");
        sendto(d, packet, strlen(packet + 4) + 5, 0, (SOCKADDR *)&tmp->addr, sizeof(SOCKADDR));
        close(d);
        free(arg);
    }

    return NULL;
}

void *WriteThread(void *arg)
{
    return NULL;
}

int main()
{
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SOCKADDR_IN saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(9999);
    saddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    if (bind(s, (SOCKADDR *)&saddr, sizeof(SOCKADDR)) == 0)
    {
        char buffer[1024] = {0};
        SOCKADDR_IN caddr;
        int clen = sizeof(SOCKADDR);
        int r = recvfrom(s, buffer, sizeof(buffer), 0, (SOCKADDR *)&caddr, &clen);
        if (r > 0)
        {
            unsigned short opcode = (buffer[0] << 8) | buffer[1];
            if (opcode == RRQ)
            {
                pthread_t tid;
                ARG *arg = (ARG *)calloc(1, sizeof(ARG));
                arg->addr = caddr;
                memcpy(arg->buffer, buffer, sizeof(buffer));
                pthread_create(&tid, NULL, ReadThread, arg);
            } // else if (opcode == WRQ)
            //{
            //    pthread_t tid;
            //    pthread_create(&tid, NULL, WriteThread, NULL);
            //}
        }
    }
    else
    {
        printf("Failed to bind!\n");
    }
    close(s);
    getchar();
}