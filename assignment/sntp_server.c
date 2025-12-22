#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define NTP_TIMESTAMP_DELTA 2208988800ull
#define TEST_PORT 9000

typedef struct {
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

void quit_with_error(char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

int main() {
  int sockfd;
  struct sockaddr_in srv_addr, client_addr;
  ntp_packet pkt;
  socklen_t client_len = sizeof(client_addr);

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    quit_with_error("Không thể tạo socket");

  memset(&srv_addr, 0, sizeof(srv_addr));
  srv_addr.sin_family = AF_INET;
  srv_addr.sin_addr.s_addr = INADDR_ANY;
  srv_addr.sin_port = htons(TEST_PORT);

  if (bind(sockfd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0)
    quit_with_error("Không thể bind socket");

  printf("SNTP Server đang lắng nghe tại cổng %d...\n", TEST_PORT);

  while (1) {
    int bytes = recvfrom(sockfd, &pkt, sizeof(ntp_packet), 0,
                         (struct sockaddr *)&client_addr, &client_len);
    if (bytes < 0)
      continue;

    printf("Nhận request từ Client: %s\n", inet_ntoa(client_addr.sin_addr));

    time_t now = time(NULL);
    uint32_t ntp_now = htonl(now + NTP_TIMESTAMP_DELTA);

    pkt.orig_tm_s = pkt.tx_tm_s;
    pkt.orig_tm_f = pkt.tx_tm_f;

    pkt.rx_tm_s = ntp_now;
    pkt.rx_tm_f = 0;

    pkt.tx_tm_s = ntp_now;
    pkt.tx_tm_f = 0;

    pkt.li_vn_mode = 0x24;

    pkt.stratum = 1;
    pkt.precision = -6;

    sendto(sockfd, &pkt, sizeof(ntp_packet), 0, (struct sockaddr *)&client_addr,
           client_len);
  }

  return 0;
}
