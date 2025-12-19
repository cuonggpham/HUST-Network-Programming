#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>

#define NTP_TIMESTAMP_DELTA 2208988800ull
#define TEST_PORT 9000

// Cấu trúc gói tin NTP
typedef struct
{
    uint8_t li_vn_mode;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t ref_id;
    uint32_t ref_tm_s;
    uint32_t ref_tm_f;
    uint32_t orig_tm_s;
    uint32_t orig_tm_f;
    uint32_t rx_tm_s;
    uint32_t rx_tm_f;
    uint32_t tx_tm_s;
    uint32_t tx_tm_f;
} ntp_packet;

void exit_with_error(char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int main()
{
    int sockfd;
    struct sockaddr_in server_addr;
    ntp_packet req_packet;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        exit_with_error("Không thể tạo socket");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(TEST_PORT);

    memset(&req_packet, 0, sizeof(ntp_packet));

    req_packet.li_vn_mode = 0x23;

    req_packet.tx_tm_s = htonl(time(NULL) + NTP_TIMESTAMP_DELTA);

    if (sendto(sockfd, &req_packet, sizeof(ntp_packet), 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        exit_with_error("Lỗi khi truyền request");

    printf("Đang gửi yêu cầu tới server...\n");

    socklen_t addr_len = sizeof(server_addr);
    if (recvfrom(sockfd, &req_packet, sizeof(ntp_packet), 0,
                 (struct sockaddr *)&server_addr, &addr_len) < 0)
        exit_with_error("Không thể nhận phản hồi từ server");

    uint32_t ntp_time = ntohl(req_packet.tx_tm_s);
    time_t unix_time = (time_t)(ntp_time - NTP_TIMESTAMP_DELTA);

    printf("Server trả về (Mode: %d, Stratum: %d)\n",
           req_packet.li_vn_mode & 0x07, req_packet.stratum);

    printf("Thời gian server: %s", ctime(&unix_time));

    close(sockfd);
    return 0;
}
