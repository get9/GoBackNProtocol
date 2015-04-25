#include <stdio.h>
#include <signal.h>
#include <time.h>


// Creates a timer that, when armed, responds to signal 'sig'
int create_timer(timer_t *timerid, int sig)
{
    if (timerid == NULL) {
        return -1;
    }
    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = sig;
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
    struct itimerspec its;
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
    struct itimerspec its;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 0;
    if (timer_settime(*timerid, 0, &its, NULL) == -1) {
        perror("timer_settime");
        return -1;
    }
    return 0;
}
