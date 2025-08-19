#include "ping.h"

int resolve_hostname(const char *hostname, struct in_addr *addr) {
    struct addrinfo hints, *res;
    int ret;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      // Force IPv4
    hints.ai_socktype = SOCK_RAW;   // Raw socket

    ret = getaddrinfo(hostname, NULL, &hints, &res);
    if (ret != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        return -1;
    }

    *addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;

    freeaddrinfo(res);
    return 0;
}

unsigned short in_cksum(unsigned short *addr, int count) {
    unsigned int sum = 0;
    
    while (count > 0) {
        if (count != 1)
            sum += *addr++;
        else
            sum += *(unsigned char *)addr++;
        while (sum > UINT16_MAX)
            sum = (sum >> 16) + (sum & 0xffff);
        count -= 2;
    }
    return (unsigned short)(~sum);
}

void cleanup(int sig) {
    (void)sig;
    printf("\n--- %s ping statistics ---\n", inet_ntoa(*(struct in_addr *)&dest_addr));
    
    float loss = 0.0;
    if (tx_count > 0)
        loss = 100.0 * (tx_count - rx_count) / tx_count;
    
    printf("%d packets transmitted, %d received, %.1f%% packet loss\n",
            tx_count, rx_count, loss);
    
    if (sock >= 0) 
        close(sock);
    exit(0);
}

int init_socket() {
    int s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (s < 0) {
        perror("socket");
        fprintf(stderr, "You need to be root to create raw sockets!\n");
        return -1;
    }
    
    // Disable timeout
    // struct timeval tv_out;
    // tv_out.tv_sec = TIMEOUT_SEC;
    // tv_out.tv_usec = 0;
    // if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out)) < 0) {
    //     perror("setsockopt SO_RCVTIMEO");
    //     close(s);
    //     return -1;
    // }
    
    return s;
}

void prep_packet(char *sendbuf, int seq) {
    memset(sendbuf, 0, PKT_SIZE);
    struct icmp *icmp_pkt = (struct icmp *)sendbuf;
    
    icmp_pkt->icmp_type = ICMP_ECHO;
    icmp_pkt->icmp_code = 0;
    icmp_pkt->icmp_id = getpid() & 0xFFFF;
    icmp_pkt->icmp_seq = seq;
    
    memset(sendbuf + sizeof(struct icmphdr), 0x42, PKT_SIZE - sizeof(struct icmphdr));
    
    icmp_pkt->icmp_cksum = 0;
    icmp_pkt->icmp_cksum = in_cksum((unsigned short *)icmp_pkt, PKT_SIZE);
}

int send_packet(int sock, char *sendbuf, struct sockaddr_in *dest) {
    int bytes = sendto(sock, sendbuf, PKT_SIZE, 0, 
                      (struct sockaddr *)dest, sizeof(*dest));
    if (bytes < 0) {
        perror("sendto failed");
    }
    return bytes;
}

int receive_packet(int sock, char *recvbuf, size_t bufsize, struct sockaddr_in *from) {
    socklen_t fromlen = sizeof(*from);
    int bytes = recvfrom(sock, recvbuf, bufsize, 0,
                        (struct sockaddr *)from, &fromlen);
    return bytes;
}

void process_reply(char *recvbuf, int bytes, struct sockaddr_in *from, 
                   int seq, struct timeval *tv_start, struct timeval *tv_end) {
    (void)seq;
    struct ip *ip_hdr = (struct ip *)recvbuf;
    int hlen = ip_hdr->ip_hl << 2;
    struct icmp *icmp_reply = (struct icmp *)(recvbuf + hlen);
    
    if (icmp_reply->icmp_type == ICMP_ECHOREPLY && 
        icmp_reply->icmp_id == (getpid() & 0xFFFF)) {
        
        rx_count++;
        
        double rtt = (tv_end->tv_sec - tv_start->tv_sec) * 1000.0 +
                (tv_end->tv_usec - tv_start->tv_usec) / 1000.0;
        
        printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
            bytes - hlen,
            inet_ntoa(from->sin_addr),
            icmp_reply->icmp_seq,
            ip_hdr->ip_ttl,
            rtt);
    }
}

int ping_loop(int sock, struct sockaddr_in *dest) {
    char sendbuf[PKT_SIZE];
    char recvbuf[PKT_SIZE + sizeof(struct ip)];
    struct sockaddr_in from;
    struct timeval tv_start, tv_end;
    int bytes;
    
    while (1) {
        prep_packet(sendbuf, tx_count++);
        
        gettimeofday(&tv_start, NULL);
        
        bytes = send_packet(sock, sendbuf, dest);
        if (bytes < 0) {
            continue;
        }
        
        bytes = receive_packet(sock, recvbuf, sizeof(recvbuf), &from);
        
        gettimeofday(&tv_end, NULL);
        
        if (bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("Request timeout for icmp_seq=%d\n", tx_count - 1);
            } else {
                perror("recvfrom error");
            }
        } else {
            process_reply(recvbuf, bytes, &from, tx_count - 1, &tv_start, &tv_end);
        }
        
        sleep(1);
    }
    
    return 0;
}

void handleFlag(char c) {
    printf("Flag is %c\n", c);
}

void help() {
    printf(
            "Usage: ping [OPTION...] HOST ...\n"
            "Send ICMP ECHO_REQUEST packets to network hosts.\n"
            "\n"
            "Options valid:\n"
            "\n"
            " -v,             verbose output\n"
            " -?              give this help list\n"
            "\n"
        );
    exit(0);
}

void verifyAddr(char *argv) {
    if (inet_pton(AF_INET, argv, &dest_addr) <= 0) {
        struct in_addr resolved;
        if (resolve_hostname(argv, &resolved) < 0) {
            fprintf(stderr, "Could not resolve hostname: %s\n", argv);
            exit(1);
        }
        dest_addr = resolved.s_addr;
    }
}

void parsing(char **argv, int argc, char *flags, char *addr) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j] != '\0'; j++) {
                char currentChar = argv[i][j];

                if (currentChar == '?')
                    help();

                if (strchr(FLAGS_MANDA, currentChar) == NULL) {
                    fprintf(stderr, "Invalid flag: -%c\n", currentChar);
                    exit(1);
                }

                if (strchr(flags, currentChar) == NULL) {
                    size_t len = strlen(flags);
                    if (len + 1 < sizeof(flags)) {
                        flags[len] = currentChar;
                        flags[len + 1] = '\0';
                    }
                }
            }
        } else if (strlen(addr) == 0)
            addr = argv[i];
    }
    printf("FLAGS %s\n", flags);
    printf("addrs %s\n", addr);
    printf("------------------------------------\n");
    verifyAddr(addr);
}

int main(int argc, char *argv[]) {
    char flags[FLAGS_MANDA_LEN] = "";
    char *addr = "";

    parsing(argv, argc, flags, addr);

    sock = init_socket();
    if (sock < 0) {
        return 1;
    }
    signal(SIGINT, cleanup);

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = dest_addr;

    if (strlen(flags)) {
        printf("PING %s (%s): %d data bytes, id 0x%04x = %d\n",
            argv[1],
            inet_ntoa(*(struct in_addr *)&dest_addr),
            PKT_SIZE - (int)sizeof(struct icmphdr),
            getpid() & 0xFFFF,
            getpid() & 0xFFFF);

    } else {
        printf("PING %s (%s): %ld data bytes\n", 
            argv[1], 
            inet_ntoa(*(struct in_addr *)&dest_addr), 
            PKT_SIZE - sizeof(struct icmphdr));
    }
    
    return ping_loop(sock, &dest);
}