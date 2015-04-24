#include "data.h"

#pragma once

#define MAXBUFSIZE 512


// The type of message
enum msgtype_t {
    DATA_MSG = 1,
    TEAR_DOWN_MSG = 4,
};

// Layout of the message being sent
struct msg_t {
    enum msgtype_t type;
    int seq_no;
    int len;
    char data[MAXBUFSIZE];
};
