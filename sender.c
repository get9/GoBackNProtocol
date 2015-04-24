#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
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

    // Networking code to set up address/socket
    struct addrinfo hints;
    struct addrinfo *server_info;
    struct addrinfo *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    int err = getaddrinfo(serverip, server_port, &hints, &server_info);
    if (err != 0) {
        fprintf(stderr, "[error]: getaddrinfo: %s\n", gai_strerror(err));
        exit(1);
    }

    // Loop through to find socket to bind to
    int sock = 0;
    for (p = server_info; p != NULL; p = p->ai_next) {
        if ((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("sender: socket");
            continue;
        }
        break;
    }

    // Did we bind to a socket?
    if (p == NULL) {
        fprintf(stderr, "[error]: sender could not get socket\n");
        exit(1);
    }
    freeaddrinfo(server_info);

    // Establish signal handler for timer functions
    struct sigaction sa;
    if (establish_handler(&sa, SIGRTMIN, handler) == -1) {
        fprintf(stderr, "[error]: couldn't establish handler\n");
        exit(1);
    }
    sigset_t mask;
    if (block_signal(SIGRTMIN, &mask) == -1) {
        fprintf(stderr, "[error]: couldn't block signal\n");
        exit(1);
    }

    // Variables controlling window size
    char *bufstart = NULL;
    int32_t window_start = 0;
    int32_t window_end = window_size;
    int32_t nextseqnum = 1;

    // Send all packets between window_start and window_end
    for (int32_t i = window_start; i < window_end; ++i, ++nextseqnum) {
        struct packet_t packet;
        bufstart = g_buffer + i * chunk_size;
        if (make_packet(&packet, i, chunk_size, bufstart) == -1) {
            fprintf(stderr, "[sender]: couldn't make packet\n");
            break;
        }

        if (send_packet(&packet, sock, p) == -1) {
            fprintf(stderr, "[sender]: couldn't send packet\n");
            break;
        }
    }
    
    // Start timer for 3s
    timer_t timerid;
    if (create_timer(&timerid, SIGRTMIN) == -1) {
        fprintf(stderr, "[sender]: couldn't create timer\n");
        exit(1);
    }
    if (arm_timer(&timerid, 3) == -1) {
        fprintf(stderr, "[sender]: couldn't arm timer\n");
        exit(1);
    }

    // Main loop that processes recv's and new sends
    while (bufstart < (g_buffer + G_BUF_LEN)) {
        ack_t ack;
        if (recv_ack(ack, sock, p) == -1) {
            fprintf(stderr, "[error]: couldn't receive ACK\n");
            exit(1);
        }
    }
}
