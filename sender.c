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

    // Main loop to send entire data buffer
    int num_packets = G_BUF_LEN / chunk_size;
    for (int p = 0; p < num_packets; ++p) {

        // Send all packets in the range [base, window_size)
        for (int i = base; i < window_size && i < num_packets; ++i) {
            // Make packet
            struct msg_t packet;
            if (make_packet(&packet, i, chunk_size, (g_buffer + i * chunk_size)) == -1) {
                fprintf(stderr, "couldn't make packet\n");
                break;
            }

            // Serialize the packet into an array of bytes
            size_t packet_len = sizeof(packet.type) + 2*sizeof(int) + packet.len;
            uint8_t *packet_buf = malloc(packet_len * sizeof(uint8_t));
            uint8_t *ptr = serialize_packet(packet_buf, &packet);
            ssize_t send_len = sendto(sock, packet_buf, sizeof(packet_buf), 0, p->ai_addr, p->ai_addrlen);
            if (send_len == -1) {
                perror("sendto");
                exit(1);
            }
        }


    }
}
