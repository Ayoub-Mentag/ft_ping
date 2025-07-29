#ifndef PING_H
#define PING_H

#include <stdint.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <sys/time.h>
#include <netinet/in.h>

#define PAYLOAD_SIZE 56

typedef struct s_icmp_packet {
    struct icmphdr header;
    char payload[PAYLOAD_SIZE];
} t_icmp_packet;

unsigned short calculate_checksum(void *data, int len);
void build_icmp_packet(t_icmp_packet *pkt, uint16_t id, uint16_t seq);
int send_icmp_packet(int sockfd, struct sockaddr_in *dest, t_icmp_packet *pkt);
int receive_icmp_reply(int sockfd, struct timeval *start, struct timeval *end);

#endif
