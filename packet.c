#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "packet.h"


// Create a packet with data starting at 'buf'. buf or packet cannot be NULL
int make_packet(struct packet_t *packet, int type, int seq_no, int len, char *buf)
{
    if (packet == NULL) {
        fprintf(stderr, "[make_packet]: packet was NULL\n");
        return -1;
    } else if (buf == NULL) {
        fprintf(stderr, "[make_packet]: buf was NULL\n");
        return -1;
    }

    if (len == 0) {
        packet->type = 4;
        packet->len = len;
        return 0;
    } else if (len > MAXBUFSIZE) {
        fprintf(stderr, "[make_packet]: packet data too large!\n");
        return -1;
    } else {
        packet->type = 1;
        packet->seq_no = seq_no;
        packet->len = len;
        // XXX This shouldn't copy any null byte over...
        strncpy(packet->data, buf, len);
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
int send_ack(struct ack_t *ack, int sock, struct addrinfo *addr)
{
    if (ack == NULL) {
        fprintf(stderr, "[send_ack]: ack was NULL\n");
        return -1;
    } else if (addr == NULL) {
        fprintf(stderr, "[send_ack]: addr was NULL\n");
        return -1;
    }
    uint8_t buf[8];
    uint8_t *ptr = NULL;
    ptr = serialize_int(buf, ack->type);
    ptr = serialize_int(buf, ack->ack_no);
    ssize_t send_len = sendto(sock, buf, sizeof(buf), 0, addr->ai_addr, addr->ai_addrlen);
    if (send_len == -1) {
        perror("[send_ack]: sendto");
        return -1;
    }
    printf("--------- SEND ACK %d\n", ack->ack_no);
    return 0;
}

// Send the packet by first serializing it and then sending it over the network
int send_packet(struct packet_t *packet, int sock, struct addrinfo *addr)
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
        return -1;
    }
    ssize_t send_len = sendto(sock, buf, packet_len, 0, addr->ai_addr, addr->ai_addrlen);
    if (send_len == -1) {
        perror("[send_packet]: sendto");
        return -1;
    }
    printf("SEND PACKET %d\n", packet->seq_no);
    free(buf);
    return 0;
}

int recv_packet(struct packet_t *packet, int sock, struct sockaddr *addr)
{
    if (packet == NULL) {
        fprintf(stderr, "[recv_packet]: packet was NULL\n");
        return -1;
    } else if (addr == NULL) {
        fprintf(stderr, "[recv_packet]: addr was NULL\n");
        return -1;
    }
    // We know a packet cannot be larger than this
    size_t maxlen = 3 * sizeof(int) + MAXBUFSIZE;
    uint8_t buf[maxlen];
    socklen_t addrlen = (socklen_t) sizeof(*addr);
    printf("right before recvfrom() [packet]\n");
    ssize_t recv_len = recvfrom(sock, buf, maxlen, 0, addr, &addrlen);
    printf("right after recvfrom() [packet]\n");
    if (recv_len == -1) {
        perror("[recv_packet]: recvfrom");
        return -1;
    }
    if (deserialize(buf, packet) == -1) {
        fprintf(stderr, "[recv_packet]: couldn't deserialize from network\n");
        return -1;
    }
    printf("RECEIVED PACKET %d\n", packet->seq_no);
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
    socklen_t maxlen = (socklen_t) (2 * sizeof(int));
    uint8_t *buf = malloc(maxlen * sizeof(uint8_t));
    ssize_t recv_len = recvfrom(sock, buf, sizeof(buf), 0, addr, &maxlen);
    if (recv_len == -1) {
        perror("[recv_ack]: recvfrom");
        free(buf);
        return -1;
    }
    buf = deserialize_int(buf, &ack->type);
    buf = deserialize_int(buf, &ack->ack_no);
    free(buf);
    printf("-------- RECEIVE ACK %d\n", ack->ack_no);
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
    serialbuf = serialize_int(serialbuf, packet->type);
    serialbuf = serialize_int(serialbuf, packet->seq_no);
    serialbuf = serialize_int(serialbuf, packet->len);
    strncpy((char *)serialbuf, packet->data, packet->len);
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
    serialbuf = deserialize_int(serialbuf, &packet->type);
    serialbuf = deserialize_int(serialbuf, &packet->seq_no);
    serialbuf = deserialize_int(serialbuf, &packet->len);
    strncpy(packet->data, (char *)serialbuf, packet->len);
    return 0;
}

// Deserializes an integer from the network. Assumes both pointers are not NULL
uint8_t *deserialize_int(uint8_t *serialbuf, int *val)
{
    *val = (serialbuf[0] << 24) | (serialbuf[1] << 16) | (serialbuf[2] << 8) | serialbuf[3];
    return serialbuf + 4;
}
