#include <sys/types.h>
#include <inttypes.h>
#include <netdb.h>

#pragma once

#define MAXBUFSIZE 512


// Layout of the message being sent
struct packet_t {
    int type;
    int seq_no;
    int len;
    char data[MAXBUFSIZE];
};

// Layout of ACKs
struct ack_t {
    int type;
    int ack_no;
};

void print_packet(struct packet_t pkt);
void print_ack(struct ack_t ack);
int make_packet(struct packet_t *packet, int type, int seq_no, int len, char *buf);
int make_ack(struct ack_t *ack, int type, int ack_no);
int send_ack(struct ack_t *ack, int sock, struct addrinfo *addr, size_t addrlen);
int send_packet(struct packet_t *packet, int sock, struct addrinfo *addr);
int recv_packet(struct packet_t *packet, int sock, struct sockaddr *addr, size_t *addrlen);
int recv_ack(struct ack_t *ack, int sock, struct sockaddr *addr);
uint8_t *serialize_int(uint8_t *serialbuf, int val);
int serialize(uint8_t *serialbuf, struct packet_t *packet);
int deserialize(uint8_t *serialbuf, struct packet_t *packet);
uint8_t *deserialize_int(uint8_t *serialbuf, int *val);
