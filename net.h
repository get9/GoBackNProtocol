#include <sys/types.h>
#include <sys/socket.h>

#pragma once

#define MAX_CONNECTIONS 5
#define BUFSIZE 512


enum conn_type {
    CONN_TYPE_TCP,
    CONN_TYPE_UDP
};

int create_socket(char *port, int num_conn, enum conn_type ct);
void *get_addr_struct(struct sockaddr *client_addr);