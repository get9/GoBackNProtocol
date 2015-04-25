#include "netdb.h"

#pragma once


// Timeout var tells whether there was a timeout
extern int g_timeout;

// Helpful typedef of handler function
typedef void (*handlerfunc_t)(int, siginfo_t *, void *);

void handler(int sig, siginfo_t *si, void *uc);
int establish_handler(struct sigaction *sa, int sig, handlerfunc_t handler);
int block_signal(int sig, sigset_t *mask);
int unblock_signal(int sig, sigset_t *mask);
