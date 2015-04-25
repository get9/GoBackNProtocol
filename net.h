#include <sys/types.h>
#include <sys/socket.h>
#include <inttypes.h>

#pragma once

#define MAX_CONNECTIONS 5
#define BUFSIZE 512


enum conn_type {
    CONN_TYPE_TCP,
    CONN_TYPE_UDP
};

int create_socket(char *port, int num_conn, enum conn_type ct);
int get_addr_sock(struct addrinfo **p, int *sock, char *serverip, char *server_port);
void *get_addr_struct(struct sockaddr *client_addr);
