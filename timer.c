#include <stdio.h>
#include <sys/time.h>
#include <sys/socket.h>


// Sets timeout value for socket 'sock' for 'timeout_val'
// (in seconds)
int set_timeout(int sock, int timeout_val)
{
    struct timeval timeout;
    timeout.tv_sec = timeout_val;
    timeout.tv_usec = 0;
    int ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (ret == -1) {
        fprintf(stderr, "[set_timeout]: couldn't set timeout for sock\n");
        return -1;
    } else {
        return 0;
    }
}

// Disables timeout for socket 'sock'
int disable_timeout(int sock)
{
    if (set_timeout(sock, 0) == -1) {
        fprintf(stderr, "[disable_timeout]: couldn't disable timeout for sock\n");
        return -1;
    } else {
        return 0;
    }
}
