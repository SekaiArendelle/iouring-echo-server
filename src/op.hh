#pragma once

#include <cstdlib>

enum class OpType : unsigned {
    accept = 0,
    recv,
    send,
};

struct AcceptOp {
    sockaddr_storage buffer;
    socklen_t len;
};

struct RecvOp {
    static constexpr size_t len{128};

    int conn_sock;
    char buffer[len]{};

    RecvOp(int conn_sock_) noexcept
        : conn_sock(conn_sock_) {
    }
};

struct SendOp {
    RecvOp* recv_op;
};
