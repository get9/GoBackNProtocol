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
#include "tcp.h"
#include "util.h"
#include "http.h"

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
    if (listen(sock, max_conns) == -1) {
        die("listen() failed");
    }
    
    return sock;
}

// Accepts connections from TCP socket
int accept_tcp_connection(int server_socket)
{
    // Creates place to store client addr info and tries to accept the connection
    struct sockaddr_storage client_addr;
    unsigned int client_addr_len = sizeof(client_addr);
    int client_socket = accept(server_socket, (struct sockaddr *) &client_addr,
                               &client_addr_len);
    if (client_socket == -1) {
        die("accept() failed");
    }

    // Neat trick to get the correct sockaddr_in struct for IPV4/IPV6
    void *addr_struct = get_addr_struct((struct sockaddr *) &client_addr);
    if (addr_struct == NULL) {
        die("get_addr_struct() failed");
    }

    // Print out connection information
    char addr_str[INET6_ADDRSTRLEN];
    inet_ntop(client_addr.ss_family, addr_struct, addr_str, sizeof(addr_str));
    printf("got connection from: %s\n", addr_str);
    return client_socket;
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

// Process a TCP connection from a client
void process_http_connection(int client_socket, char *http_root)
{
    // Get the request from the client
    char *msgbuf = NULL;
    uint64_t msgbuf_len = 0;
    if (get_request(client_socket, &msgbuf, &msgbuf_len) == -1) {
        die("get_request() failed");
    }

    // Parse the request and fill the struct http_req_t
    struct http_req_t req;
    parse_request(msgbuf, msgbuf_len, &req);
    free(msgbuf);

    // Check if requesting root dir; if so, set to default path
    if (strncmp(req.file, "/", strlen(req.file)) == 0) {
        char defaultpath[] = "/index.html";
        free(req.file);
        req.file = strdup(defaultpath);
    }

    // Construct full path for the requested file
    char *fullpath = join_paths(http_root, req.file);
    free(req.file);
    req.file = fullpath;
    printf("%s: %s\n", req.method, req.file);

    // Construct the response data
    struct http_resp_t resp;
    fill_response_data(req, &resp);
    printf("Status: %s, Length: %lu\n", resp.rc, resp.content_len);
    
    // Make the response string
    char *resp_str = NULL;
    if (strncmp(resp.rc, "404 Not Found", strlen(resp.rc)) == 0) {
        resp_str = make_404();
    } else {
        resp_str = make_response_str(&resp);
    }

    // Finally, send the response back to the server and close connection
    printf("%s\n", resp_str);
    if (send(client_socket, resp_str, resp.content_len, 0) == -1) {
        free(resp_str);
        destroy_http_req_t(&req);
        destroy_http_resp_t(&resp);
        die("send() failed");
    }

    // Cleanup
    free(resp_str);
    destroy_http_req_t(&req);
    destroy_http_resp_t(&resp);

    close(client_socket);
}

// Gets request from the client and puts the received bytes in msgbuf with length msgbuf_len
int get_request(int client_socket, char **msgbuf, uint64_t *msgbuf_len)
{
    // Handle input validation, etc
    if (msgbuf_len == NULL) {
        return -1;
    }

    // Alloc memory for new message; initialized to BUFSIZE constant
    *msgbuf = malloc(BUFSIZE * sizeof(char));
    *msgbuf_len = BUFSIZE;
    //uint64_t content_len = 0;
    int64_t recv_size = 0;

    // Receive bytes
    recv_size = recv(client_socket, *msgbuf, *msgbuf_len, 0);
    if (recv_size == -1) {
        die("recv() failed");
    }
    /*
    do {
        recv_size = recv(client_socket, *msgbuf + content_len, *msgbuf_len - content_len, 0);

        // Since client_socket is non-blocking, recv_size will be -1 when no data received
        if (recv_size < 0) {
            break;
        }

        content_len += (uint64_t) recv_size;

        // Need to resize msgbuf if necessary
        if ((uint64_t) recv_size == *msgbuf_len) {
            uint64_t multiplier = 2;
            char *tmpbuf = resize_buf(*msgbuf, *msgbuf_len, multiplier);    
            if (tmpbuf == NULL) {
                free(*msgbuf);
                die("resize_buf() failed");
            }
            *msgbuf = tmpbuf;
            *msgbuf_len *= multiplier;
        }
    } while (recv_size > 0);
    */

    return 0;
}

// Populate the response struct
void fill_response_data(struct http_req_t req, struct http_resp_t *resp)
{
    uint64_t cont_len = 0;
    char *content = NULL;
    if (get_content(req.file, &content, &cont_len) == -1 || !file_exists(req.file)) {
        // 404
        *resp = (struct http_resp_t) {
            .rc = "404 Not Found",
            .content_type = NULL,
            .content = NULL,
            .content_len = 0,
            .vers = get_vers(req.vers)
        };
    } else if (file_exists(req.file)) {
        *resp = (struct http_resp_t) {
            .rc = get_response_code(req.file),
            .content_type = get_content_type(req.file),
            .content_len = cont_len,
            .content = content,
            .vers = get_vers(req.vers)
        };
    }
}

// Craft response string to send back to the client; updates resp length with headers length
char *make_response_str(struct http_resp_t *resp)
{
    // snprintf with 0 as second arg returns size it would have written
    int32_t strsize = snprintf(NULL, 0,
                           "%s %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n",
                           resp->vers, resp->rc, resp->content_type, resp->content_len);
    if (strsize < 0) {
        return NULL;
    }
    // Write header
    resp->content_len = (uint32_t) strsize + resp->content_len;
    char *resp_str = malloc(resp->content_len * sizeof(char));
    int32_t newsize = snprintf(resp_str, (uint32_t) strsize,
                           "%s %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n",
                           resp->vers, resp->rc, resp->content_type, resp->content_len);
    if (newsize < 0) {
        resp->content_len = 0;
        free(resp_str);
        return NULL;
    }
    // Need to put back the newline that snprintf() stripped off to write \0
    resp_str[strsize-1] = '\n';
    
    // Write content
    memcpy(resp_str + strsize, resp->content, resp->content_len);
    return resp_str;
}

// Make a generic 404 response string
char *make_404(void)
{
    // Need to ensure we don't get the null byte at the end and that this is a dynamic string
    char errmsg[] = "HTTP/1.0 404 Not Found\r\nConnection: close\r\n\r\n<html><head><title>404 Not Found</title></head><body><p>404 Not Found: the requested resource could not be found</p></body></html>";
    char *m_errmsg = malloc(strlen(errmsg) * sizeof(char));
    strncpy(m_errmsg, errmsg, strlen(errmsg));
    return m_errmsg;
}
