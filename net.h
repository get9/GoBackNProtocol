#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "http.h"


#pragma once


#define MAX_CONNECTIONS 5
#define BUFSIZE 512


enum conn_type {
    CONN_TYPE_TCP,
    CONN_TYPE_UDP
};

int create_tcp_socket(char *port, int num_conn);
void *get_addr_struct(struct sockaddr *client_addr);
