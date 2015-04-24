#include <signal.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "server.h"
#include "util.h"
#include "net.h"


char g_buf[BUFSIZE];

// Server main execution loop
void run_server(char *port, double loss_rate)
{
    // Create a UDP server socket
    int server_socket = create_socket(port, MAX_CONNECTIONS, CONN_TYPE_UDP);

    // Var for connecting machine info
    struct sockaddr_storage connecting_addr;

    // Server mainloop
    socklen_t addr_len = sizeof(connecting_addr);
    while (true) {
        // Get bytes from the wire
        ssize_t numbytes = recvfrom(server_socket, g_buf, BUFSIZE-1, 0,
                                (struct sockaddr *)&connecting_addr, &addr_len);
        if (numbytes == -1) {
            perror("recvfrom");
            exit(1);
        }
        
        char s[INET6_ADDRSTRLEN];
        printf("server: got packet from %s\n",
               inet_ntop(connecting_addr.ss_family,
               get_addr_struct((struct sockaddr *)&connecting_addr),
               s, sizeof(s)));
        printf("server: packet is %d bytes long\n", numbytes);
        buf[numbytes] = '\0';
        printf("server: packet contains \"%s\"\n", buf);
    }
}
