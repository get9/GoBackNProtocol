#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "net.h"


// Gets the addr and port for a given server_ip and server_port
int get_addr_sock(struct addrinfo **p, int *sock, char *serverip, char *server_port)
{
    if (*p == NULL) {
        fprintf(stderr, "[get_addr_port]: *p was NULL\n");
        return -1;
    } else if (sock == NULL) {
        fprintf(stderr, "[get_addr_port]: port was NULL\n");
        return -1;
    }

    // Networking code to set up address/socket
    struct addrinfo hints;
    struct addrinfo *server_info;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    if (serverip == NULL) {
        hints.ai_flags = AI_PASSIVE;
    }

    int err = getaddrinfo(serverip, server_port, &hints, &server_info);
    if (err != 0) {
        fprintf(stderr, "[get_addr_port]: getaddrinfo: %s\n", gai_strerror(err));
        return -1;
    }

    // Loop through to find socket to bind to
    for (*p = server_info; *p != NULL; *p = (*p)->ai_next) {
        if ((*sock = socket((*p)->ai_family, (*p)->ai_socktype, (*p)->ai_protocol)) == -1) {
            perror("[get_addr_port]: sender: socket");
            continue;
        }

        // Bind to socket, but only if it's server that's calling
        if (serverip == NULL) {
            if (bind(*sock, (*p)->ai_addr, (*p)->ai_addrlen) == -1) {
                close(*sock);
                perror("[get_addr_port]: bind");
                continue;
            }
        }
        break;
    }

    // Did we bind to a socket?
    if (*p == NULL) {
        fprintf(stderr, "[get_addr_port]: sender could not get socket\n");
        return -1;
    }

    freeaddrinfo(server_info);
    return 0;
}

// Handy function to get the correct struct for IPV4 or IPV6 calls
void *get_addr_struct(struct sockaddr *client_addr)
{
    if (client_addr == NULL) {
        return (void *) NULL;
    } else if (client_addr->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) client_addr)->sin_addr);
    } else {
        return &(((struct sockaddr_in6 *) client_addr)->sin6_addr);
    }
}
