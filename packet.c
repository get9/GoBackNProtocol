#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <stdbool.h>
#include "packet.h"


bool is_lost(double loss_rate)
{
    double rv = drand48();
    return (rv < loss_rate);
}

// Prints packet information
void print_packet(struct packet_t pkt)
{
    printf("type   = %d\n", pkt.type);
    printf("seq_no = %d\n", pkt.seq_no);
    printf("len    = %d\n", pkt.len);
    printf("data   = ");
    for (int i = 0; i < pkt.len; ++i) {
        printf("%c", pkt.data[i]);
    }
    printf("\n");
}

// Prints ack information
void print_ack(struct ack_t ack)
{
    printf("type   = %d\n", ack.type);
    printf("ack_no = %d\n", ack.ack_no);
}

// Create a packet with data starting at 'buf'. buf or packet cannot be NULL
int make_packet(struct packet_t *packet, int type, int seq_no, int len, char *buf)
{
    if (packet == NULL) {
        fprintf(stderr, "[make_packet]: packet was NULL\n");
        return -1;
    }

    if (len == 0) {
        packet->type = 4;
        packet->len = len;
        packet->seq_no = -1;
        return 0;
    } else if (len > MAXBUFSIZE) {
        fprintf(stderr, "[make_packet]: packet data too large!\n");
        return -1;
    } else {
        packet->type = 1;
        packet->seq_no = seq_no;
        packet->len = len;
        for (int i = 0; i < len; ++i) {
            packet->data[i] = buf[i];
        }
    }
    return 0;
}

// Makes an ACK packet
int make_ack(struct ack_t *ack, int type, int ack_no)
{
    if (ack == NULL) {
        fprintf(stderr, "[make_ack]: ack was NULL\n");
        return -1;
    }
    ack->type = type;
    ack->ack_no = ack_no;
    return 0;
}

// Sends ACK packet
int send_ack(struct ack_t *ack, int sock, struct sockaddr *addr)
{
    if (ack == NULL) {
        fprintf(stderr, "[send_ack]: ack was NULL\n");
        return -1;
    } else if (addr == NULL) {
        fprintf(stderr, "[send_ack]: addr was NULL\n");
        return -1;
    }
    int32_t buflen = 2 * sizeof(int);
    uint8_t *buf = malloc(buflen);
    uint8_t *ptr = buf;
    ptr = serialize_int(ptr, ack->type);
    ptr = serialize_int(ptr, ack->ack_no);
    ssize_t send_len = sendto(sock, buf, buflen, 0, addr, sizeof(*addr));
    if (send_len == -1) {
        perror("[send_ack]: sendto");
        return -1;
    }
    return 0;
}

// Send the packet by first serializing it and then sending it over the network
int send_packet(struct packet_t *packet, int sock, struct sockaddr *addr)
{
    if (packet == NULL) {
        fprintf(stderr, "[send_packet]: packet was NULL\n");
        return -1;
    } else if (addr == NULL) {
        fprintf(stderr, "[send_packet]: addr was NULL\n");
        return -1;
    }
    size_t packet_len = 3 * sizeof(int) + packet->len;
    uint8_t *buf = malloc(packet_len * sizeof(uint8_t));
    if (serialize(buf, packet) == -1) {
        fprintf(stderr, "[send_packet]: couldn't serialize packet\n");
        free(buf);
        return -1;
    }
    if (buf == NULL) {
        fprintf(stderr, "[send_packet]: couldn't serialize packet\n");
        free(buf);
        return -1;
    }
    size_t addrlen = sizeof(*addr);
    ssize_t send_len = sendto(sock, buf, packet_len, 0, addr, addrlen);
    if (send_len == -1) {
        free(buf);
        perror("[send_packet]: sendto");
        return -1;
    }
    free(buf);
    return 0;
}

int recv_packet(struct packet_t *packet, int sock, struct sockaddr *addr, socklen_t *addrlen, double loss_rate)
{
    if (packet == NULL) {
        fprintf(stderr, "[recv_packet]: packet was NULL\n");
        return -1;
    } else if (addr == NULL) {
        fprintf(stderr, "[recv_packet]: addr was NULL\n");
        return -1;
    } else if (addrlen == NULL) {
        fprintf(stderr, "[recv_packet]: addrlen was NULL\n");
        return -1;
    }
    // We know a packet cannot be larger than this
    size_t maxlen = 3 * sizeof(int) + MAXBUFSIZE;
    uint8_t *buf = malloc(maxlen * sizeof(uint8_t));
    
    // Loop until we can actually receive something (due to loss_rate)
    while (is_lost(loss_rate)) {
        ssize_t recv_len = recvfrom(sock, buf, maxlen, 0, addr, addrlen);
        if (recv_len == -1) {
            return -1;
        }
    }
    ssize_t recv_len = recvfrom(sock, buf, maxlen, 0, addr, addrlen);
    if (recv_len == -1) {
        return -1;
    }
    if (deserialize(buf, packet) == -1) {
        fprintf(stderr, "[recv_packet]: couldn't deserialize packet\n");
        free(buf);
        return -1;
    }
    free(buf);
    return 0;
}

int recv_ack(struct ack_t *ack, int sock, struct sockaddr *addr)
{
    if (ack == NULL) {
        fprintf(stderr, "[recv_ack]: packet was NULL\n");
        return -1;
    } else if (addr == NULL) {
        fprintf(stderr, "[recv_ack]: addr was NULL\n");
        return -1;
    }
    // We know a packet cannot be larger than this
    size_t maxlen = (socklen_t) (2 * sizeof(int));
    uint8_t *buf = malloc(maxlen * sizeof(uint8_t));
    socklen_t addrlen = sizeof(*addr);
    ssize_t recv_len = recvfrom(sock, buf, maxlen, 0, addr, &addrlen);
    if (recv_len == -1) {
        free(buf);
        return -1;
    }
    uint8_t *tmp = buf;
    tmp = deserialize_int(tmp, &ack->type);
    tmp = deserialize_int(tmp, &ack->ack_no);
    free(buf);
    return 0;
}

// Serialize packet into a single buffer of bytes. Assumes the serialbuf is long enough
int serialize(uint8_t *serialbuf, struct packet_t *packet)
{
    if (packet == NULL) {
        fprintf(stderr, "[serialize]: packet was NULL\n");
        return -1;
    } else if (serialbuf == NULL) {
        fprintf(stderr, "[serialize]: serialbuf was NULL\n");
        return -1;
    }
    uint8_t *tmp = serialbuf;
    tmp = serialize_int(tmp, packet->type);
    tmp = serialize_int(tmp, packet->seq_no);
    tmp = serialize_int(tmp, packet->len);
    strncpy((char *)tmp, packet->data, packet->len);
    return 0;
}

// Serialize an int into network-byte order. Increases buffer pointer to next
// available location to insert data to.
uint8_t *serialize_int(uint8_t *serialbuf, int val)
{
    serialbuf[0] = val >> 24;
    serialbuf[1] = val >> 16;
    serialbuf[2] = val >> 8;
    serialbuf[3] = val;
    return serialbuf + 4;
}

// Deserializes a packet from the network.
int deserialize(uint8_t *serialbuf, struct packet_t *packet)
{
    if (packet == NULL) {
        fprintf(stderr, "[deserialize]: packet was NULL\n");
        return -1;
    } else if (serialbuf == NULL) {
        fprintf(stderr, "[deserialize]: serialbuf was NULL\n");
        return -1;
    }
    uint8_t *tmp = serialbuf;
    tmp = deserialize_int(tmp, &packet->type);
    tmp = deserialize_int(tmp, &packet->seq_no);
    tmp = deserialize_int(tmp, &packet->len);
    strcpy(packet->data, (char *)tmp);
    return 0;
}

// Deserializes an integer from the network. Assumes both pointers are not NULL
uint8_t *deserialize_int(uint8_t *serialbuf, int *val)
{
    *val = (serialbuf[0] << 24) | (serialbuf[1] << 16) | (serialbuf[2] << 8) | serialbuf[3];
    return serialbuf + 4;
}
