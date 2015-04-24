#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "sighandler.h"
#include "timer.h"
#include "data.h"
#include "net.h"
#include "packet.h"


int main(int argc, char **argv)
{
    // Verify and parse args
    if (argc < 4) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "    %s server_IP server_port chunk_size window_size\n", argv[0]);
        exit(1);
    }
    char *serverip = argv[1];
    char *tmp;
    long int p = strtol(argv[2], &tmp, 10);
    if (p < 0 || p > USHRT_MAX) {
        fprintf(stderr, "[error]: port %ld is invalid", p);
        exit(1);
    }
    uint16_t server_port = (uint16_t) p;
    long int c = strtol(argv[3], &tmp, 10);
    if (c < 0 || c > MAXBUFSIZE) {
        fprintf(stderr, "[error]: chunk_size should be between 0 and MAXBUFSIZE\n");
        exit(1);
    }
    int32_t chunk_size = (int32_t) c;
    long int w = strtol(argv[4], &tmp, 10);
    // XXX No error checking for window_size?
    int32_t window_size = (int32_t) w;

    // 'base' is the smallest un-ACK'd sequence number
    int32_t base = 0;
}
