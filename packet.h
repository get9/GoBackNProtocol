#include "data.h"

#pragma once

#define MAXBUFSIZE 512


// Layout of the message being sent
struct msg_t {
    int type;
    int seq_no;
    int len;
    char data[MAXBUFSIZE];
};

// Create a packet with data starting at 'buf'. buf or packet cannot
// be NULL
// If data 
int make_packet(struct msg_t *packet, int type, int seq_no, int len, char *buf)
{
    if (packet == NULL) {
        fprintf(stderr, "packet was NULL\n");
        return -1;
    } else if (buf == NULL) {
        fprintf(stderr, "buf was NULL\n");
        return -1;
    }

    if (len == 0) {
        packet->type = TEAR_DOWN_MSG;
        packet->len = len;
        return 0;
    } else if (len > MAXBUFSIZE) {
        fprintf(stderr, "packet data too large!\n");
        return -1;
    } else {
        packet->type = DATA_MSG;
        packet->seq_no = seq_no;
        packet->len = len;
        // XXX This shouldn't copy any null byte over...
        strncpy(packet->data, buf, len);
    }
    return 0;
}

// Serialize packet into a single buffer of bytes
uint8_t *serialize_packet(uint8_t *serialbuf, struct msg_t *packet)
{
    if (packet == NULL) {
        fprintf(stderr, "serialize_packet: packet was NULL\n");
        return NULL;
    } else if (serialbuf == NULL) {
        fprintf(stderr, "serialize_packet: serialbuf was NULL\n");
        return NULL;
    }
    serialbuf = serialize_int(packet->type);
    serialbuf = serialize_int(packet->seq_no);
    serialbuf = serialize_int(packet->len);
    strncpy(serialbuf, packet->data, packet->len);
    return serialbuf + packet->len;
}

// Serialize an int into network-byte order
uint8_t *serialize_int(uint8_t *serialbuf, int val)
{
    serialbuf[0] = value >> 24;
    serialbuf[1] = value >> 16;
    serialbuf[2] = value >> 8;
    serialbuf[3] = value;
    return serialbuf + 4;
}
