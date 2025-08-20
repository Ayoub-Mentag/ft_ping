#ifndef PING_H
#define PING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>

#define PKT_SIZE 64
#define HOSTNAME_MAX 256
#define FLAGS_MANDA  "v"
#define FLAGS_BONUS "vncWwp"
#define FLAGS_MANDA_LEN  1
#define FLAGS_BONUS_LEN  6


typedef struct {
    int sock;
    int tx_count;
    int rx_count;
    unsigned int dest_addr;
} PingData;

PingData g_ping;


typedef struct {
    bool n;  // numeric
    bool v;  // verbose
    int c;   // count
    int W;   // timeout for each reply
    int w;   // delay
    char *p; // payload
} FlagsData;

#endif
