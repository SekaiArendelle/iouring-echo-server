#pragma once

#include <cstdlib>

enum class OpType : unsigned {
    accept = 0,
    recv,
    send,
};

struct Op {
    OpType op_type;
    int fd;
    sockaddr_storage buffer;
    socklen_t len;
};
