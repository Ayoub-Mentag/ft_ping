#include "ping.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

int main() {
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    inet_pton(AF_INET, "8.8.8.8", &dest.sin_addr);  // or resolve from argv

    t_icmp_packet pkt;
    uint16_t id = getpid() & 0xFFFF;

    build_icmp_packet(&pkt, id, 1);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    if (send_icmp_packet(sockfd, &dest, &pkt) < 0) {
        perror("sendto");
        return 1;
    }

    if (receive_icmp_reply(sockfd, &start, &end) > 0) {
        double rtt = (end.tv_sec - start.tv_sec) * 1000.0 +
                     (end.tv_usec - start.tv_usec) / 1000.0;
        printf("RTT: %.3f ms\n", rtt);
    } else {
        printf("No reply received.\n");
    }

    close(sockfd);
    return 0;
}
