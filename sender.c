#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>
#include <math.h>
#include <netdb.h>
#include "data.h"
#include "timer.h"
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
    long int x = strtol(argv[2], NULL, 10);
    if (x < 0 || x > USHRT_MAX) {
        fprintf(stderr, "[error]: port %ld is invalid\n", x);
        exit(1);
    }
    char *server_port = argv[2];
    long int c = strtol(argv[3], NULL, 10);
    if (c < 1 || c > MAXBUFSIZE) {
        fprintf(stderr, "[error]: chunk_size should be between 1 and MAXBUFSIZE\n");
        exit(1);
    }
    int32_t chunk_size = (int32_t) c;
    long int w = strtol(argv[4], NULL, 10);
    int32_t window_size = (int32_t) w;

    printf("server_IP   = %s\n", serverip);
    printf("server_port = %s\n", server_port);
    printf("chunk_size  = %d\n", chunk_size);
    printf("window_size = %d\n", window_size);

    // Get network information
    struct sockaddr addr;
    int sock;
    if (get_addr_sock(&addr, &sock, serverip, server_port) == -1) {
        fprintf(stderr, "[sender]: couldn't get socket or addrinfo\n");
        exit(1);
    }

    // Set initial timeout
    if (set_timeout(sock, TIMEOUT_SEC) == -1) {
        exit(1);
    }

    // Variables controlling window size. bufptr is the pointer to the start
    // of where to pull data from g_buffer
    char *bufptr = g_buffer;
    int32_t base = 0;
    int32_t nextseqnum = 0;
    int32_t num_packets = (int32_t) ceil((strlen(g_buffer) + 1) / chunk_size);
    int32_t last_ack = 0;
    int32_t retransmissions = 0;
    int32_t timedout = false;
    struct packet_t *sentpkts = malloc(window_size * sizeof(struct packet_t));

    // Send out window_size packets initially
    for (int i = 0; i < window_size; ++i) {
        // Make a new packet
        int init_pktlen = chunk_size;
        struct packet_t pkt;
        int ret = make_packet(&pkt, 1, nextseqnum, init_pktlen, bufptr);
        if (ret == -1) {
            fprintf(stderr, "[sender]: couldn't make packet %d\n", nextseqnum);
            goto cleanup_and_exit;
        }

        // If succesfully sent, cache the packet in the array of packets which
        // represents our window.
        if (send_packet(&pkt, sock, &addr) == -1) {
            fprintf(stderr, "[sender]: couldn't send packet %d\n", nextseqnum);
            goto cleanup_and_exit;
        }
        printf("SEND PACKET %d\n", pkt.seq_no);
        sentpkts[nextseqnum % window_size] = pkt;
        nextseqnum++;
        bufptr += chunk_size;
    }

    // Main loop for sender activity
    while (*bufptr != '\0') {
        // If there was a timeout, resend the packets from base to nextseqnum - 1
        if (timedout) {
            for (int i = base; i < nextseqnum; ++i) {
                struct packet_t oldpkt = sentpkts[i % window_size];
                if (send_packet(&oldpkt, sock, &addr) == -1) {
                    fprintf(stderr, "[sender]: failed to resend packet %d\n", oldpkt.seq_no);
                    break;
                }
                printf("SEND PACKET %d\n", oldpkt.seq_no);
            }

            // Reset state variables and record retransmissions
            timedout = false;
            retransmissions++;
            if (retransmissions > 10) {
                fprintf(stderr, "[failure]: retried 10 times, could not send packets\n");
                goto cleanup_and_exit;
            }
        }

        // Can send a new packet because there's room in the window. Make a new packet
        // and send it.
        else if (nextseqnum < base + window_size) {
            retransmissions = 0;
            // Size the packet. If this is the last packet, it could potentially be smaller
            size_t pktlen = chunk_size;
            if (strlen(bufptr) < chunk_size) {
                pktlen = strlen(bufptr) + 1;
            }

            // Make a new packet
            struct packet_t pkt;
            int ret = make_packet(&pkt, 1, nextseqnum, pktlen, bufptr);
            if (ret == -1) {
                fprintf(stderr, "[sender]: couldn't make packet %d\n", nextseqnum);
                break;
            }

            // If succesfully sent, cache the packet in the array of packets which
            // represents our window.
            if (send_packet(&pkt, sock, &addr) == -1) {
                fprintf(stderr, "[sender]: couldn't send packet %d\n", nextseqnum);
                break;
            }
            printf("SEND PACKET %d\n", pkt.seq_no);
            sentpkts[nextseqnum % window_size] = pkt;

            // Increment bufptr so that it points to the start of the next data to send
            bufptr += pktlen;

            // If our base is the same as nextseqnum, we need to arm the timer
            if (base == nextseqnum) {
                if (set_timeout(sock, TIMEOUT_SEC) == -1) {
                    fprintf(stderr, "[sender]: couldn't set timeout\n");
                    goto cleanup_and_exit;
                }
            }
            nextseqnum++;
        }

        // Receive an ACK and check if we can stop the timer. Update base and
        // nextseqnum accordingly
        struct ack_t ack;
        if (recv_ack(&ack, sock, &addr) != -1) {
            base = ack.ack_no + 1;
            last_ack = ack.ack_no;

            // If we've reached the nextseqnum, there are no outstanding packets
            // so disable timer
            if (base == nextseqnum) {
                if (disable_timeout(sock) == -1) {
                    goto cleanup_and_exit;
                }
            }
            printf("--------RECEIVED ACK %d\n", ack.ack_no + 1);
        } else {
            if (errno == EAGAIN) {
                timedout = true;
            } else {
                fprintf(stderr, "[sender]: couldn't receive ACK\n");
                goto cleanup_and_exit;
            }
        }
    }

    // Need to wait to see if we got all ACKs
    while (last_ack < num_packets) {
        struct ack_t ack;
        if (recv_ack(&ack, sock, &addr) == -1) {
            fprintf(stderr, "[sender]: couldn't receive remaining ACKs\n");
            goto cleanup_and_exit;
        }
        printf("--------RECEIVED ACK %d\n\n", ack.ack_no);
        last_ack = ack.ack_no;
    }

    // After sending all packets and receiving all ACKs, construct tear-down
    // message (type=8 and len=0)
    struct packet_t tear_down_pkt;
    printf("Sending tear-down packet\n");
    if (make_packet(&tear_down_pkt, 4, 0, 0, NULL) == -1) {
        fprintf(stderr, "[sender]: couldn't construct tear-down packet\n");
        goto cleanup_and_exit;
    }

    // Set timeout on socket (just to make sure it's still set for the
    // final transmissions)
    if (set_timeout(sock, TIMEOUT_SEC) == -1) {
        goto cleanup_and_exit;
    }
    
    // Retransmit this packet 10 times, unless it gets an ACK of type=8
    for (int i = 0; i < 10; ++i) {
        if (send_packet(&tear_down_pkt, sock, &addr) == -1) {
            fprintf(stderr, "[sender]: couldn't send tear-down packet\n");
            goto cleanup_and_exit;
        }
        printf("SEND TEAR-DOWN PACKET\n");
        struct ack_t tear_down_ack;
        if (recv_ack(&tear_down_ack, sock, &addr) == -1) {
            fprintf(stderr, "[sender]: couldn't receive tear-down ack\n");
            goto cleanup_and_exit;
        }
        printf("-------- RECEIVED TEAR-DOWN ACK\n");
        if (tear_down_ack.type == 8) {
            break;
        }
    }

    cleanup_and_exit:
        free(sentpkts);
        exit(1);
}
