#include "ping.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>

unsigned short calculate_checksum(void *data, int len) {
    unsigned short *buf = data;
    unsigned int sum = 0;

    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }

    if (len == 1) {
        sum += *(unsigned char *)buf;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return ~sum;
}

void build_icmp_packet(t_icmp_packet *pkt, uint16_t id, uint16_t seq) {
    memset(pkt, 0, sizeof(*pkt));
    pkt->header.type = ICMP_ECHO;
    pkt->header.code = 0;
    pkt->header.un.echo.id = id;
    pkt->header.un.echo.sequence = seq;
    strcpy(pkt->payload, "hello from ft_ping");
    pkt->header.checksum = 0;
    pkt->header.checksum = calculate_checksum(pkt, sizeof(*pkt));
}

int send_icmp_packet(int sockfd, struct sockaddr_in *dest, t_icmp_packet *pkt) {
    return sendto(sockfd, pkt, sizeof(*pkt), 0, (struct sockaddr *)dest, sizeof(*dest));
}

int receive_icmp_reply(int sockfd, struct timeval *start, struct timeval *end) {
    (void) start;
    char buffer[1024];
    struct sockaddr_in from;
    socklen_t addr_len = sizeof(from);

    ssize_t bytes = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                             (struct sockaddr *)&from, &addr_len);
    if (bytes < 0)
        return -1;

    gettimeofday(end, NULL);

    struct iphdr *ip = (struct iphdr *)buffer;
    struct icmphdr *icmp = (struct icmphdr *)(buffer + (ip->ihl * 4));

    if (icmp->type == ICMP_ECHOREPLY) {
        printf("Received ICMP Echo Reply!\n");
        return 1;
    }
    return 0;
}



int ft_ping(char *ip_address) {
    // Open socket
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    // Set up destination
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    inet_pton(AF_INET, ip_address, &dest.sin_addr);  // or resolve from argv


    // Build packet
    t_icmp_packet pkt;
    uint16_t id = getpid() & 0xFFFF;
    build_icmp_packet(&pkt, id, 1);


    // 
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