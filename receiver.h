#pragma once

// Passing char * so port can get passed to getaddrinfo correctly
_Noreturn void run_server(char *, char *);

// Signal handler functions
void init_signalhandler(void);
void sigchld_handler(int sig);
