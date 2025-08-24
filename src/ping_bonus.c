#include "ping.h"

int resolve_hostname(const char *hostname, struct in_addr *addr) {
    struct addrinfo hints, *res;
    int ret;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;

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
    printf("\n--- %s ping statistics ---\n", g_ping.addr);
    
    float loss = 0.0;
    if (g_ping.tx_count > 0)
        loss = 100.0 * (g_ping.tx_count - g_ping.rx_count) / g_ping.tx_count;

    printf("%d packets transmitted, %d received, %.1f%% packet loss\n",
            g_ping.tx_count, g_ping.rx_count, loss);

    printf("round-trip min/avg/max/stddev = %.2f/%.2f/%.2f/%.2f ms\n",
        g_ping.min,
        g_ping.avg / g_ping.rx_count,
        g_ping.max,
        0.0);


    if (g_ping.sock >= 0) 
        close(g_ping.sock);
    exit(0);
}

int init_socket(ArgsData* argsData) {
    struct timeval tv_out;
    int s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (s < 0) {
        perror("socket");
        fprintf(stderr, "You need to be root to create raw sockets!\n");
        return -1;
    }
    
    if (argsData->W) {
        tv_out.tv_sec = argsData->W;
        tv_out.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out)) < 0) {
            perror("setsockopt SO_RCVTIMEO");
            close(s);
            return -1;
        }
    }

    return s;
}

void print_packet(const char *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", (unsigned char)buf[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    if (len % 16 != 0)
        printf("\n");
}



void prep_packet(char *sendbuf, int seq, char* payload) {
    size_t data_len = strlen(payload);

    memset(sendbuf, 0, PKT_SIZE);
    struct icmp *icmp_pkt = (struct icmp *)sendbuf;

    icmp_pkt->icmp_type = ICMP_ECHO;
    icmp_pkt->icmp_code = 0;
    icmp_pkt->icmp_id = getpid() & 0xFFFF;
    icmp_pkt->icmp_seq = seq;

    char *data_ptr = sendbuf + sizeof(struct icmp);

    size_t data_space = PKT_SIZE - sizeof(struct icmp);

    if (data_len > 0) {
        for (size_t i = 0; i < data_space; i++) {
            data_ptr[i] = payload[i % data_len];
        }
    }

    icmp_pkt->icmp_cksum = 0;
    icmp_pkt->icmp_cksum = in_cksum((unsigned short *)icmp_pkt, PKT_SIZE);
}


int send_packet(int sock, char *sendbuf, struct sockaddr_in *dest) {
    int bytes = sendto(sock, sendbuf, PKT_SIZE, 0, 
                      (struct sockaddr *)dest, sizeof(*dest));
    if (bytes < 0)
        perror("sendto failed");
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
        
        g_ping.rx_count++;
        
        float rtt = (tv_end->tv_sec - tv_start->tv_sec) * 1000.0 +
                (tv_end->tv_usec - tv_start->tv_usec) / 1000.0;
        if (g_ping.min > rtt)
            g_ping.min = rtt;

        if (g_ping.max < rtt)
            g_ping.max = rtt;

        g_ping.avg += rtt;

        printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
            bytes - hlen,
            inet_ntoa(from->sin_addr),
            icmp_reply->icmp_seq,
            ip_hdr->ip_ttl,
            rtt);
    }
}

void printHeader(ArgsData *argsData) {
    int data_bytes = PKT_SIZE - (int)sizeof(struct icmphdr);
    char *ip_str = inet_ntoa(*(struct in_addr *)&g_ping.dest_addr);

    printf("PING %s (%s): %d data bytes", g_ping.addr, ip_str, data_bytes);

    if (argsData->v) {
        int pid = getpid() & 0xFFFF;
        printf(", id 0x%04x = %d", pid, pid);
    }

    printf("\n");
}

int ping_loop(int sock, struct sockaddr_in *dest, ArgsData *argsData) {
    char sendbuf[PKT_SIZE];
    char recvbuf[PKT_SIZE + sizeof(struct ip)];
    struct sockaddr_in from;
    struct timeval tv_start, tv_end;
    int bytes;
    bool infinite = !argsData->c && !argsData->w;
    time_t start;

    printHeader(argsData);
    start = time(NULL);
    if (argsData->c && argsData->w)
        argsData->c = argsData->w;

    while (infinite || argsData->c-- > 0 || time(NULL) - start < argsData->w) {
        prep_packet(sendbuf, g_ping.tx_count++, argsData->p);
        gettimeofday(&tv_start, NULL);
        bytes = send_packet(sock, sendbuf, dest);
        if (bytes < 0)
            continue;
        bytes = receive_packet(sock, recvbuf, sizeof(recvbuf), &from);
        gettimeofday(&tv_end, NULL);
        if (bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                printf("Request timeout for icmp_seq=%d\n", g_ping.tx_count - 1);
            else
                perror("recvfrom error");
        } else
            process_reply(recvbuf, bytes, &from, g_ping.tx_count - 1, &tv_start, &tv_end);
        sleep(1);
    }
    cleanup(0);
    return 0;
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
            "-n               Numeric output only\n"
            "-c               Send only a specified number of packets\n"
            "-W               Timeout per packet\n"
            "-w               Timeout total\n"
            "-p               Fill the ICMP payload with a repeating byte pattern\n"
            "\n"
        );
    exit(0);
}

void verifyAddr(ArgsData *argsData) {
    if (argsData->n) {
        if (inet_pton(AF_INET, argsData->addr, &g_ping.dest_addr) <= 0) {
            fprintf(stderr, "Invalid numeric IP: %s\n", argsData->addr);
            exit(1);
        }
        return;
    }

    if (inet_pton(AF_INET, argsData->addr, &g_ping.dest_addr) <= 0) {
        struct in_addr resolved;
        if (resolve_hostname(argsData->addr, &resolved) < 0) {
            fprintf(stderr, "Could not resolve hostname: %s\n", argsData->addr);
            exit(1);
        }
        g_ping.dest_addr = resolved.s_addr;
    }

    if (argsData->addr[strlen(argsData->addr) - 1] == '.')
        argsData->addr[strlen(argsData->addr) - 1] = '\0';
    g_ping.addr = argsData->addr;
}

void debugFlags(const ArgsData *f) {
    printf("---- ArgsData Debug ----\n");
    printf("n (numeric): %s\n", f->n ? "true" : "false");
    printf("v (verbose): %s\n", f->v ? "true" : "false");
    printf("c (count):   %d\n", f->c);
    printf("W (timeout): %d\n", f->W);
    printf("w (delay):   %d\n", f->w);
    printf("p (pattern): %s\n", f->p ? f->p : "(null)");
    printf("-------------------------\n");
}

int isNumber(const char *s) {
    if (s == NULL || *s == '\0') return 0;
    for (int i = 0; s[i]; i++) {
        if (!isdigit((unsigned char)s[i])) return 0;
    }
    return 1;
}

void initFlag(ArgsData *argsData, char currentChar, char *tmp) {
    switch (currentChar) {
        case 'c': 
            argsData->c = atoi(tmp);
            break;
        case 'w': 
            argsData->c = atoi(tmp);
            break;
        case 'W': 
            argsData->c = atoi(tmp);
            break;
        default:
            break;
    }
}

void parsing(int argc, char **argv, ArgsData *argsData) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j] != '\0'; j++) {
                char currentChar = argv[i][j];

                if (currentChar == '?')
                    help();

                if (strchr("nvcWwp", currentChar) == NULL) {
                    fprintf(stderr, "Invalid flag: -%c\n", currentChar);
                    exit(1);
                }

                char *tmp = NULL;

                switch (currentChar) {
                    case 'n':
                        argsData->n = true;
                        break;

                    case 'v':
                        argsData->v = true;
                        break;

                    case 'c':
                    case 'W':
                    case 'w':
                    case 'p':
                        if (argv[i][j+1] != '\0') {
                            tmp = argv[i] + j + 1;
                        } else {
                            if (i + 1 >= argc) {
                                fprintf(stderr, "Option -%c requires an argument\n", currentChar);
                                exit(1);
                            }
                            tmp = argv[i + 1];
                            i++;
                        }
                        j = (int)strlen(argv[i]) - 1;

                        if (currentChar == 'p')
                            argsData->p = tmp;
                        else if (isNumber(tmp))
                            initFlag(argsData, currentChar, tmp);
                        else {
                            fprintf(stderr, "Invalid argument for -%c: %s\n", currentChar, tmp);
                            exit(1);
                        }
                        break;

                    default:
                        break;
                }
            }
        } else if (!strlen(argsData->addr)) {
            argsData->addr = argv[i];
        }
    }

    if (!strlen(argsData->addr)) {
        fprintf(stderr, "Destination address required\n");
        exit(1);
    }
    verifyAddr(argsData);
}


void init(ArgsData* argsData) {
    argsData->n = 0;
    argsData->v = 0;
    argsData->c = 0;
    argsData->W = 0;
    argsData->w = 0;
    argsData->p = "";
    argsData->addr = "";
    g_ping.sock = -1;
    g_ping.tx_count = 0;
    g_ping.rx_count = 0;
    g_ping.min = FLT_MAX;
    g_ping.max = FLT_MIN;
    g_ping.avg = 0;
}

int main(int argc, char *argv[]) {
    ArgsData argsData;
    struct sockaddr_in dest;

    init(&argsData);
    parsing(argc, argv, &argsData);

    g_ping.sock = init_socket(&argsData);
    if (g_ping.sock < 0)
        return 1;

    signal(SIGINT, cleanup);

    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = g_ping.dest_addr;

    return ping_loop(g_ping.sock, &dest, &argsData);
}