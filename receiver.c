#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>
#include <netdb.h>

#include "net.h"
#include "timer.h"
#include "data.h"
#include "packet.h"


int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage:\n");
        printf("    %s port_no [loss_rate]\n", argv[0]);
        exit(1);
    }

    // Parse/validate args
    char *port = argv[1];
    printf("port = %s\n", port);
    long int x = strtol(argv[1], NULL, 10);
    if (x < 0 || x > USHRT_MAX) {
        fprintf(stderr, "[error]: port %ld is invalid\n", x);
        exit(1);
    }
    double loss_rate = 0.0;
    if (argc == 3) {
        loss_rate = strtof(argv[2], NULL);
        if (loss_rate < 0.0 || loss_rate > 1.0) {
            fprintf(stderr, "[error]: loss_rate must be between 0 and 1\n");
            exit(1);
        }
    }

    // Get a socket to connect to
    int sock;
    struct addrinfo *p;
    struct sockaddr_storage their_addr;
    if (get_addr_sock(&p, &sock, NULL, port) == -1) {
        fprintf(stderr, "[error]: unable to get socket\n");
        exit(1);
    }

    // Len of connecting address
    size_t their_addr_len;

    // Last packet received
    int packet_received = -1;

    // Buffer to store data in
    char buf[G_BUF_LEN];
    char *bufstart = buf;

    // Main loop of execution - runs until we get an error or tear-down msg
    while (true) {
        // Receive a packet
        struct packet_t pkt;
        if (recv_packet(&pkt, sock, (struct sockaddr *)&their_addr, &their_addr_len) == -1) {
            fprintf(stderr, "[receiver]: couldn't receive packet\n");
            exit(1);
        }
        print_packet(pkt);

        // Check if this is the tear-down message. If so, get out of loop.
        if (pkt.type == 4) {
            break;
        }

        // Check if this is the next packet in the sequence. If so, adjust
        // packet_received appropriately and copy data to the buffer
        else if (pkt.seq_no == packet_received + 1) {
            packet_received++;
            strncpy(bufstart, pkt.data, pkt.len);
            bufstart += pkt.len;
        }

        // Send ACK
        struct ack_t ack;
        if (make_ack(&ack, 2, packet_received) == -1) {
            fprintf(stderr, "[receiver]: couldn't construct ACK\n");
            exit(1);
        }
        print_ack(ack);
        if (send_ack(&ack, sock, (struct addrinfo *)&their_addr, their_addr_len) == -1) {
            fprintf(stderr, "[receiver]: couldn't send ACK %d\n", ack.ack_no);
            exit(1);
        }
    }

    // Construct ACK to tear-down message and send
    struct ack_t tear_down_ack;
    if (make_ack(&tear_down_ack, 8, 0) == -1) {
        fprintf(stderr, "[receiver]: couldn't construct tear-down ACK\n");
        exit(1);
    }
    if (send_ack(&tear_down_ack, sock, (struct addrinfo *)&their_addr, their_addr_len) == -1) {
        fprintf(stderr, "[receiver]: couldn't send tear-down ACK\n");
        exit(1);
    }

    // Timer for 7 seconds. Additionally, set a timeout on the socket so that
    // we don't exceed the timeout by not receiving any packets
    if (set_timeout(sock, 7) == -1) {
        fprintf(stderr, "[receiver]: unable to set timeout\n");
        exit(1);
    }
    clock_t start = clock();
    int msec = (clock() - start) * 1000 / CLOCKS_PER_SEC;
    while (msec < 7000) {
        struct packet_t pkt;
        if (recv_packet(&pkt, sock, (struct sockaddr *)&their_addr, &their_addr_len) == -1) {
            fprintf(stderr, "[receiver]: couldn't receive any packets after data\n");
            break;
        }
        
        // Only ACK if it's a tear-down packet
        if (pkt.type == 4) {
            if (send_ack(&tear_down_ack, sock, (struct addrinfo *)&their_addr, their_addr_len) == -1) {
                fprintf(stderr, "[receiver]: couldn't send tear-down ACK\n");
                break;
            }
        }
    }

    return 0;
}
