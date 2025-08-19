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
#define FLAGS_MANDA  "v?"
#define FLAGS_BONUS "v?ncWwp"
#define FLAGS_MANDA_LEN  2
#define FLAGS_BONUS_LEN  7

int sock = -1;
int tx_count = 0;
int rx_count = 0;
unsigned int dest_addr;

#endif
