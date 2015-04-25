#include <time.h>

#pragma once

#define TIMEOUT_SEC 3


int set_timeout(int sock, int timeout_val);
int disable_timeout(int sock);

/*
int create_timer(timer_t *timerid, int sig);
int arm_timer(timer_t *timerid, int sec);
int disarm_timer(timer_t *timerid);
*/
