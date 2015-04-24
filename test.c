#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


typedef void (*handlerfunc_t)(int, siginfo_t, void *);

// Creates a timer that, when armed, responds to signal 'sig'
int create_timer(timer_t *timerid, int sig)
{
    if (timerid == NULL) {
        return -1;
    }
    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_sino = sig;
    sev.sigev_value.sival_ptr = timerid;
    if (timer_create(CLOCK_MONOTONIC, &sev, timerid) == -1) {
        perror("timer_create");
        return -1;
    }
    return 0;
}

// Arms timer given by 'timerid' with time 'sec' seconds
int arm_timer(timer_t *timerid, int sec)
{
    if (timerid == NULL) {
        return -1;
    }
    struct itermspec its;
    its.it_value.tv_sec = sec;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;
    if (timer_settime(*timerid, 0, &its, NULL) == -1) {
        perror("timer_settime");
        return -1;
    }
    return 0;
}

// Disarms timer given by 'timerid'
int disarm_timer(timer_t *timerid)
{
    if (timerid == NULL) {
        return -1;
    }
    struct itermspec its;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 0;
    if (timer_settime(*timerid, 0, &its, NULL) == -1) {
        perror("timer_settime");
        return -1;
    }
    return 0;
}

// Signal handler for the timer
void handler(int sig, siginfo_t *si, void *uc)
{
    // XXX calling printf() from signal handler isn't async-safe
    printf("caught signal %d\n", sig);
    signal(sig, SIG_IGN);
}

// Establishes the handler 'handler()' for 'sa'
int establish_handler(struct sigaction *sa, int sig, handlerfunc_t handler)
{
    if (sa == NULL) {
        return -1;
    }
    sa->sa_flags = SA_SIGINFO;
    sa->sa_sigaction = *handler;
    sigemptyset(&sa->sa_mask);
    if (sigaction(sig, sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    return 0;
}

// Blocks signal 'sig'
int block_signal(int sig, sigset_t mask)
{
    sigemptyset(&mask);
    sigaddset(&mask, sig);
    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1) {
        perror("sigprocmask");
        return -1;
    }
    return 0;
}

// Unblocks signal 'sig'
int unblock_signal(int sig, sigset_t mask)
{
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1) {
        perror("sigprocmask");
        return -1;
    }
    return 0;
}

int main ()
{
    timer_t timerid;
    int sig = SIGRTMIN;
    if (create_timer(&timerid, sig) == -1) {
        fprintf(stderr, "couldn't create timer\n");
        exit(1);
    }

    struct sigaction sa;
    if (establish_handler(&sa, sig, handler) == -1) {
        fprintf(stderr, "couldn't establish handler\n");
        exit(1);
    }

    sigset_t mask;
    if (block_signal(sig, &mask) == -1) {
        fprintf(stderr, "couldn't block signal\n");
        exit(1);
    }

    if (create_timer(&timerid, sig) == -1) {
        fprintf(stderr, "couldn't create timer\n");
        exit(1);
    }

    if (arm_timer(&timerid, 3) == -1) {
        fprintf(stderr, "couldn't arm timer\n");
        exit(1);
    }

    sleep(5);

    if (unblock_signal(sig, &mask) == -1) {
        fprintf(stderr, "couldn't unblock signal\n");
        exit(1);
    }
}
