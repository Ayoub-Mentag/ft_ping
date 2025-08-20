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
    printf("\n--- %s ping statistics ---\n", inet_ntoa(*(struct in_addr *)&g_ping.dest_addr));
    
    float loss = 0.0;
    if (g_ping.tx_count > 0)
        loss = 100.0 * (g_ping.tx_count - g_ping.rx_count) / g_ping.tx_count;
    
    printf("%d packets transmitted, %d received, %.1f%% packet loss\n",
            g_ping.tx_count, g_ping.rx_count, loss);
    
    if (g_ping.sock >= 0) 
        close(g_ping.sock);
    exit(0);
}

int init_socket(FlagsData* flagsData) {
    struct timeval tv_out;
    int s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (s < 0) {
        perror("socket");
        fprintf(stderr, "You need to be root to create raw sockets!\n");
        return -1;
    }
    
    tv_out.tv_sec = flagsData->W;
    tv_out.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, flagsData->W, &tv_out, sizeof(tv_out)) < 0) {
        perror("setsockopt SO_RCVTIMEO");
        close(s);
        return -1;
    }
    
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
        
        g_ping.rx_count++;
        
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

int ping_loop(int sock, struct sockaddr_in *dest, FlagsData *flagsData) {
    char sendbuf[PKT_SIZE];
    char recvbuf[PKT_SIZE + sizeof(struct ip)];
    struct sockaddr_in from;
    struct timeval tv_start, tv_end;
    int bytes;
    bool infinite = !flagsData->c && !flagsData->w;

    while (infinite || flagsData->c-- || time(NULL) - time(NULL) < flagsData->w) {
        prep_packet(sendbuf, g_ping.tx_count++);
        
        gettimeofday(&tv_start, NULL);
        
        bytes = send_packet(sock, sendbuf, dest);
        if (bytes < 0)
            continue;
        
        bytes = receive_packet(sock, recvbuf, sizeof(recvbuf), &from);
        
        gettimeofday(&tv_end, NULL);
        
        if (bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("Request timeout for icmp_seq=%d\n", g_ping.tx_count - 1);
            } else {
                perror("recvfrom error");
            }
        } else {
            process_reply(recvbuf, bytes, &from, g_ping.tx_count - 1, &tv_start, &tv_end);
        }
        
        sleep(1);
    }
    
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

void verifyAddr(char *argv, bool n) {
    if (n) {
        if (inet_pton(AF_INET, argv, &g_ping.dest_addr) <= 0) {
            fprintf(stderr, "Invalid numeric IP: %s\n", argv);
            exit(1);
        }
        return;
    }

    if (inet_pton(AF_INET, argv, &g_ping.dest_addr) <= 0) {
        struct in_addr resolved;
        if (resolve_hostname(argv, &resolved) < 0) {
            fprintf(stderr, "Could not resolve hostname: %s\n", argv);
            exit(1);
        }
        g_ping.dest_addr = resolved.s_addr;
    }
}

// void parsing(char **argv, int argc, FlagsData *flagsData, char *addr) {
//     for (int i = 1; i < argc; i++) {
//         if (argv[i][0] == '-') {
//             for (int j = 1; argv[i][j] != '\0'; j++) {
//                 char currentChar = argv[i][j];
//                 if (currentChar == '?')
//                     help();
//                 if (strchr(FLAGS_BONUS, currentChar) == NULL) {
//                     fprintf(stderr, "Invalid flag: -%c\n", currentChar);
//                     exit(1);
//                 }
//                 switch (currentChar) {
//                     case 'n':
//                         flagsData.n = true;
//                     case 'v':
//                         flagsData.v = true;
//                         break ;
//                     case 'c':
//                         char *tmp = strlen(argv[i]) > 2 ? argv[i] + j : argv[++i];
//                         if (tmp.isNotNumber)
//                             fprintf(stderr, "Invalid option"); exit(1);
//                         flagsData.c = atoi(tmp);
//                         break ;
//                     case 'W':
//                         char *tmp = strlen(argv[i]) > 2 ? argv[i] + j : argv[++i];
//                         if (tmp.isNotNumber)
//                             fprintf(stderr, "Invalid option"); exit(1);
//                         flagsData.W = atoi(tmp);
//                         break ;
//                     case 'w':
//                         char *tmp = strlen(argv[i]) > 2 ? argv[i] + j : argv[++i];
//                         if (tmp.isNotNumber)
//                             fprintf(stderr, "Invalid option"); exit(1);
//                         flagsData.w = atoi(tmp);
//                         break ;
//                     case 'p':
//                         char *tmp = strlen(argv[i]) > 2 ? argv[i] + j : argv[++i];
//                         flagsData.p = tmp;
//                         break ;
//                     default:
//                         break ;
//                 }
//             }
//         } else if (strlen(addr) == 0)
//             addr = argv[i];
//     }
//     printf("FLAGS %s\n", flags);
//     printf("addrs %s\n", addr);
//     printf("------------------------------------\n");
//     verifyAddr(addr, flags);
// }

void debugFlags(const FlagsData *f) {
    printf("---- FlagsData Debug ----\n");
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

void parsing(int argc, char **argv, FlagsData *flagsData, char *addr) {
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
                        flagsData->n = true;
                        break;

                    case 'v':
                        flagsData->v = true;
                        break;

                    case 'c':
                    case 'W':
                    case 'w':
                    case 'p':
                        // If argument is stuck to the flag (-c10), use the rest of the string
                        if (argv[i][j+1] != '\0') {
                            tmp = argv[i] + j + 1;
                        } else {
                            // Otherwise, take next argv
                            if (i + 1 >= argc) {
                                fprintf(stderr, "Option -%c requires an argument\n", currentChar);
                                exit(1);
                            }
                            tmp = argv[i + 1];
                            i++;
                        }
                        j = (int)strlen(argv[i]) - 1;
                        if (currentChar == 'c') {
                            if (!isNumber(tmp)) {
                                fprintf(stderr, "Invalid argument for -c: %s\n", tmp);
                                exit(1);
                            }
                            flagsData->c = atoi(tmp);
                        } else if (currentChar == 'W') {
                            if (!isNumber(tmp)) {
                                fprintf(stderr, "Invalid argument for -W: %s\n", tmp);
                                exit(1);
                            }
                            flagsData->W = atoi(tmp);
                        } else if (currentChar == 'w') {
                            if (!isNumber(tmp)) {
                                fprintf(stderr, "Invalid argument for -w: %s\n", tmp);
                                exit(1);
                            }
                            flagsData->w = atoi(tmp);
                        } else if (currentChar == 'p') {
                            flagsData->p = tmp;
                        }
                        break;

                    default:
                        break;
                }
            }
        } else if (!strlen(addr)) {
            addr = argv[i];
        }
    }

    if (!strlen(addr)) {
        fprintf(stderr, "Destination address required\n");
        exit(1);
    }

    debugFlags(flagsData);
    // printf("Addr %s\n", addr);
    verifyAddr(addr, flagsData->n);
}


int main(int argc, char *argv[]) {
    // char flags[FLAGS_BONUS_LEN] = "";
    char *addr = "";
    FlagsData flagsData;
    flagsData.n = 0;
    flagsData.v = 0;
    flagsData.c = -1;
    flagsData.W = 0; // timeoute for each reply
    flagsData.w = -1;
    flagsData.p = NULL;

    parsing(argc, argv, &flagsData, addr);

    g_ping.sock = -1;
    g_ping.tx_count = 0;
    g_ping.rx_count = 0;
    
    g_ping.sock = init_socket(&flagsData);
    if (g_ping.sock < 0)
        return 1;

    signal(SIGINT, cleanup);

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = g_ping.dest_addr;

    if (flagsData.v) {
        printf("PING %s (%s): %d data bytes, id 0x%04x = %d\n",
            argv[1],
            inet_ntoa(*(struct in_addr *)&g_ping.dest_addr),
            PKT_SIZE - (int)sizeof(struct icmphdr),
            getpid() & 0xFFFF,
            getpid() & 0xFFFF);

    } else {
        printf("PING %s (%s): %ld data bytes\n", 
            argv[1], 
            inet_ntoa(*(struct in_addr *)&g_ping.dest_addr), 
            PKT_SIZE - sizeof(struct icmphdr));
    }
    
    return ping_loop(g_ping.sock, &dest, &flagsData);
}