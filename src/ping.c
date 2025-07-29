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
