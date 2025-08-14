#include "ping.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

int main(int ac, char **av) {
    if (ac != 2) {
        printf("Specify the host that you wanna ping\n");
        return 1;
    }

    int result = ft_ping(av[1]);
    return result;
}
