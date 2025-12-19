#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_PORT 69
#define BUFFER_SIZE 516   // 4 byte header + 512 byte data
#define DATA_SIZE 512

// Cac opcode cua TFTP
#define OP_RRQ 1    
#define OP_WRQ 2    
#define OP_DATA 3    
#define OP_ACK 4     
#define OP_ERROR 5   

// Ham gui goi RRQ (Read Request)
void send_rrq(int sock, struct sockaddr_in server_addr, const char *filename, const char *mode) {
    char buffer[BUFFER_SIZE];
    int len = 0;

    short opcode = htons(OP_RRQ);
    memcpy(buffer + len, &opcode, 2);
    len += 2;

    strcpy(buffer + len, filename);
    len += strlen(filename) + 1;

    strcpy(buffer + len, mode);
    len += strlen(mode) + 1;

    sendto(sock, buffer, len, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
}

// Ham gui goi WRQ (Write Request)
void send_wrq(int sock, struct sockaddr_in server_addr, const char *filename, const char *mode) {
    char buffer[BUFFER_SIZE];
    int len = 0;

    short opcode = htons(OP_WRQ);
    memcpy(buffer + len, &opcode, 2);
    len += 2;

    strcpy(buffer + len, filename);
    len += strlen(filename) + 1;

    strcpy(buffer + len, mode);
    len += strlen(mode) + 1;

    sendto(sock, buffer, len, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Cach dung: %s <ip_server> <filename> <mode>\n", argv[0]);
        printf("Mode: rrq (tai ve) | wrq (gui len)\n");
        return 1;
    }

    const char *server_ip = argv[1];
    const char *filename = argv[2];
    const char *action = argv[3];
    const char *mode = "octet"; // Truyen file nhi phan

    int sock;
    struct sockaddr_in server_addr, from_addr;
    socklen_t from_len;
    char buffer[BUFFER_SIZE];
    FILE *fp;
    int nbytes;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Khong tao duoc socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);


    // TRUONG HOP RRQ
    if (strcmp(action, "rrq") == 0) {
        send_rrq(sock, server_addr, filename, mode);
        fp = fopen(filename, "wb");
        if (!fp) {
            perror("Khong mo duoc file de ghi");
            close(sock);
            return 1;
        }

        int expected_block = 1;

        while (1) {
            from_len = sizeof(from_addr);
            nbytes = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&from_addr, &from_len);
            if (nbytes < 0) {
                perror("Loi khi nhan du lieu");
                break;
            }

            short opcode = ntohs(*(short*)buffer);

            if (opcode == OP_DATA) {
                short block = ntohs(*(short*)(buffer + 2));

                if (block == expected_block) {
                    int data_len = nbytes - 4;
                    fwrite(buffer + 4, 1, data_len, fp);

                    char ack[4];
                    *(short*)ack = htons(OP_ACK);
                    *(short*)(ack + 2) = htons(block);
                    sendto(sock, ack, 4, 0, (struct sockaddr*)&from_addr, from_len);

                    if (data_len < DATA_SIZE)
                        break;

                    expected_block++;
                }
            } else if (opcode == OP_ERROR) {
                printf("Nhan loi tu server: %s\n", buffer + 4);
                break;
            } else {
                printf("Opcode khong hop le: %d\n", opcode);
                break;
            }
        }

        fclose(fp);
        printf("Tai file '%s' thanh cong!\n", filename);
    }


    //  WRQ 
    else if (strcmp(action, "wrq") == 0) {
        send_wrq(sock, server_addr, filename, mode);

        // Mo file can gui
        fp = fopen(filename, "rb");
        if (!fp) {
            perror("Khong mo duoc file de doc");
            close(sock);
            return 1;
        }

        from_len = sizeof(from_addr);

        // Cho server gui ACK block 0 de bat dau nhan file
        nbytes = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&from_addr, &from_len);
        if (nbytes < 0) {
            perror("Khong nhan duoc ACK tu server");
            fclose(fp);
            close(sock);
            return 1;
        }

        short opcode = ntohs(*(short*)buffer);
        short block = ntohs(*(short*)(buffer + 2));

        if (opcode != OP_ACK || block != 0) {
            printf("Khong nhan duoc ACK block 0, opcode=%d, block=%d\n", opcode, block);
            fclose(fp);
            close(sock);
            return 1;
        }

        // Bat dau gui du lieu
        short block_number = 1;
        while (1) {
            char data_packet[BUFFER_SIZE];
            int data_len = fread(data_packet + 4, 1, DATA_SIZE, fp);

            *(short*)data_packet = htons(OP_DATA);
            *(short*)(data_packet + 2) = htons(block_number);

            sendto(sock, data_packet, data_len + 4, 0, (struct sockaddr*)&from_addr, from_len);

            // Cho ACK cho block vua gui
            nbytes = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&from_addr, &from_len);
            if (nbytes < 0) {
                perror("Loi khi cho ACK");
                break;
            }

            opcode = ntohs(*(short*)buffer);
            short recv_block = ntohs(*(short*)(buffer + 2));

            if (opcode == OP_ACK && recv_block == block_number) {
                if (data_len < DATA_SIZE)
                    break; // Het file
                block_number++;
            } else if (opcode == OP_ERROR) {
                printf("Nhan loi tu server: %s\n", buffer + 4);
                break;
            } else {
                printf("Phan hoi khong hop le: opcode=%d, block=%d\n", opcode, recv_block);
                break;
            }
        }

        fclose(fp);
        printf("Gui file '%s' len server thanh cong!\n", filename);
    }

    else {
        printf("Mode khong hop le!\n");
    }

    close(sock);
    return 0;
}
