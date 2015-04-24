/*
 * Much of this code was derived from Beej's Guide to Network Programming (as
 * seen here: http://beej.us/guide/bgnet/output/html/multipage/index.html
 * 
 * It is a very useful guide for writing modern networking code in C on a UNIX-
 * style system.
 *
 * All code was written by myself. While it was heavily influence by the above
 * guide, I implemented everything here, looking up functions and their use as
 * I went in order to learn more about networking code in C.
 */

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