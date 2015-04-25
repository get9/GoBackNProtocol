#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <stdlib.h>
#include "sighandler.h"


// Signal handler for the timer
extern void handler(int sig, siginfo_t *si, void *uc)
{
    // XXX calling printf() from signal handler isn't async-safe
    printf("caught signal %d\n", sig);
    g_timeout = true;
    signal(sig, SIG_IGN);
}

// Establishes the handler 'handler()' for 'sa'
int establish_handler(struct sigaction *sa, int sig, handlerfunc_t handler)
{
    if (sa == NULL) {
        return -1;
    }
    sa->sa_flags = SA_SIGINFO;
    sa->sa_sigaction = handler;
    sigemptyset(&sa->sa_mask);
    if (sigaction(sig, sa, NULL) == -1) {
        perror("sigaction");
        return -1;
    }
    return 0;
}

// Blocks signal 'sig'
int block_signal(int sig, sigset_t *mask)
{
    if (mask == NULL) {
        fprintf(stderr, "mask is NULL\n");
        return -1;
    }
    sigemptyset(mask);
    sigaddset(mask, sig);
    if (sigprocmask(SIG_SETMASK, mask, NULL) == -1) {
        perror("sigprocmask");
        return -1;
    }
    return 0;
}

// Unblocks signal 'sig'
int unblock_signal(int sig, sigset_t *mask)
{
    if (mask == NULL) {
        fprintf(stderr, "mask is NULL\n");
        return -1;
    }
    if (sigprocmask(SIG_UNBLOCK, mask, NULL) == -1) {
        perror("sigprocmask");
        return -1;
    }
    return 0;
}
