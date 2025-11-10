#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_PORT 69

// 4 byte header + 512 byte data
#define BUFFER_SIZE 516
#define DATA_SIZE 512

// opcode
#define OP_RRQ 1    
#define OP_DATA 3    
#define OP_ACK 4     
#define OP_ERROR 5   

void send_rrq(int sock, struct sockaddr_in server_addr, const char *filename, const char *mode) {
    char buffer[BUFFER_SIZE];
    int len = 0;

    // Ghi opcode vao buffer (2 byte dau)
    short opcode = htons(OP_RRQ); // chuyen sang network byte order
    memcpy(buffer + len, &opcode, 2);
    len += 2;

    // Ghi ten file vao buffer
    strcpy(buffer + len, filename);
    len += strlen(filename) + 1; // +1 de ghi ky tu ket thuc chuoi '\0'

    // Ghi che do (mode) vao buffer, vi du "octet"
    strcpy(buffer + len, mode);
    len += strlen(mode) + 1;

    // Gui goi RRQ den dia chi cua server
    sendto(sock, buffer, len, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
}

int main(int argc, char *argv[]) {
    // Chuong trinh can 2 tham so: dia chi IP server va ten file
    if (argc != 3) {
        printf("Cach dung: %s <ip_server> <filename>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    const char *filename = argv[2];
    const char *mode = "octet"; // Che do truyen file nhi phan

    int sock;
    struct sockaddr_in server_addr, from_addr;
    socklen_t from_len;
    char buffer[BUFFER_SIZE];
    FILE *fp;
    int nbytes;

    // Tao socket UDP
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Khong tao duoc socket");
        return 1;
    }

    // Cau hinh thong tin dia chi cua server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    // Gui goi yeu cau RRQ den server
    send_rrq(sock, server_addr, filename, mode);

    // Mo file de ghi du lieu nhan duoc tu server
    fp = fopen(filename, "wb");
    if (!fp) {
        perror("Khong mo duoc file de ghi");
        close(sock);
        return 1;
    }

    int expected_block = 1; // Block duoc mong doi dau tien la 1

    // Vong lap nhan du lieu tu server
    while (1) {
        from_len = sizeof(from_addr);

        // Nhan goi tin tu server (DATA hoac ERROR)
        nbytes = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&from_addr, &from_len);
        if (nbytes < 0) {
            perror("Loi khi nhan du lieu");
            break;
        }

        // Doc opcode (2 byte dau)
        short opcode = ntohs(*(short*)buffer);

        // Neu la goi DATA
        if (opcode == OP_DATA) {
            // Doc so block (2 byte tiep theo)
            short block = ntohs(*(short*)(buffer + 2));

            // Chi xu ly neu block nhan dung thu tu mong doi
            if (block == expected_block) {
                // Tinh kich thuoc du lieu (tru 4 byte header)
                int data_len = nbytes - 4;

                // Ghi du lieu vao file
                fwrite(buffer + 4, 1, data_len, fp);

                // Tao goi ACK de gui lai server
                char ack[4];
                *(short*)ack = htons(OP_ACK);
                *(short*)(ack + 2) = htons(block);

                // Gui ACK de thong bao da nhan du block nay
                sendto(sock, ack, 4, 0, (struct sockaddr*)&from_addr, from_len);

                // Neu kich thuoc du lieu < 512 thi la block cuoi => ket thuc
                if (data_len < DATA_SIZE)
                    break; 

                // Tang block mong doi len 1
                expected_block++;
            }
        }
        // Neu la goi ERROR
        else if (opcode == OP_ERROR) {
            printf("Nhan loi tu server: %s\n", buffer + 4);
            break;
        }
        // Neu opcode khong hop le
        else {
            printf("Opcode khong hop le: %d\n", opcode);
            break;
        }
    }

    // Dong file va socket
    fclose(fp);
    close(sock);
    printf("Tai file '%s' thanh cong!\n", filename);
    return 0;
}
