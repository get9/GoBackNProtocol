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
#include "util.h"


// Creates a socket and returns the socket descriptor
int create_socket(char *port, int max_conns, enum conn_type ct)
{
    // Get local machine info via getaddrinfo syscall
    struct addrinfo hints;
    struct addrinfo *llinfo;
    memset(&hints, 0, sizeof(hints)); 
    int protocol = -1;
    int socktype = -1;
    if (ct == CONN_TYPE_TCP) {
        protocol = IPPROTO_TCP;
        socktype = SOCK_STREAM;
    } else {
        protocol = IPPROTO_UDP;
        socktype = SOCK_DGRAM;
    }
    hints = (struct addrinfo) {
        .ai_family = AF_UNSPEC,
        .ai_protocol = protocol,
        .ai_socktype = socktype,
        .ai_flags = AI_PASSIVE
    };
    int status = getaddrinfo(NULL, port, &hints, &llinfo);
    if (status != 0) {
        die("getaddrinfo() failed");
    }
    
    // Create socket for incoming connections. Must loop through linked list
    // returned by getaddrinfo and try to bind to the first available result
    struct addrinfo *s = NULL;
    int sock = 0;
    for (s = llinfo; s != NULL; s = s->ai_next) {
        // Connect to the socket
        sock = socket(s->ai_family, s->ai_socktype, s->ai_protocol);
        if (sock == -1) {
            die("socket() failed");
        }

        // Set the socket option that gets around "Address already in use"
        // errors
        int tru = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &tru, sizeof(int)) == -1) {
            die("setsockopt() failed");
        }

        // Try to bind to this address; if it doesn't work, go to the next one
        if (bind(sock, s->ai_addr, s->ai_addrlen) == -1) {
            close(sock);
            perror("bind() failed");
            continue;
        }

        // Break out of loop since we got a bound address
        break;
    }

    // Check that we didn't iterate through the entire getaddrinfo linked list
    // and clean up getaddrinfo alloc
    if (s == NULL) {
        die("server could not bind any address");
    }
    freeaddrinfo(llinfo);

    // Set socket to listen for new connections
    if (ct == CONN_TYPE_TCP) {
        if (listen(sock, max_conns) == -1) {
            die("listen() failed");
        }
    }
    
    return sock;
}

// Gets the addr and port for a given server_ip and server_port
int get_addr_sock(struct addrinfo **p, int *sock, char *serverip, uint16_t server_port)
{
    if (*p == NULL) {
        fprintf(stderr, "[get_addr_port]: *p was NULL\n");
        return -1;
    } else if (port == NULL) {
        fprintf(stderr, "[get_addr_port]: port was NULL\n");
        return -1;
    }
    // Networking code to set up address/socket
    struct addrinfo hints;
    struct addrinfo *server_info;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    int err = getaddrinfo(serverip, server_port, &hints, &server_info);
    if (err != 0) {
        fprintf(stderr, "[error]: getaddrinfo: %s\n", gai_strerror(err));
        return -1;
    }

    // Loop through to find socket to bind to
    for (*p = server_info; *p != NULL; *p = *p->ai_next) {
        if ((*sock = socket(*p->ai_family, *p->ai_socktype, *p->ai_protocol)) == -1) {
            perror("sender: socket");
            continue;
        }
        break;
    }

    // Did we bind to a socket?
    if (*p == NULL) {
        fprintf(stderr, "[error]: sender could not get socket\n");
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
