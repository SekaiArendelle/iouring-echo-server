#include <cstdio>
#include <cstring>
#include <memory>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <liburing.h>

#include "op.hh"

static void queue_accept(io_uring* ring, int listen_sock) {
    AcceptOp* op = (AcceptOp*)malloc(sizeof(AcceptOp));
    if (op == nullptr) [[unlikely]] {
        __builtin_trap();
    }
    op->len = sizeof(op->buffer);

    io_uring_sqe* sqe = io_uring_get_sqe(ring);
    if (sqe == nullptr) [[unlikely]] {
        __builtin_trap();
    }

    io_uring_prep_accept(sqe, listen_sock, reinterpret_cast<sockaddr*>(__builtin_addressof(op->buffer)),
                         __builtin_addressof(op->len), 0);
    io_uring_sqe_set_data(sqe, op);
}

static void queue_recv(io_uring* ring, int conn_sock) {
    RecvOp* recv_op = (RecvOp*)malloc(sizeof(RecvOp));
    if (recv_op == nullptr) [[unlikely]] {
        __builtin_trap();
    }
    ::std::construct_at(recv_op, conn_sock);

    io_uring_sqe* sqe = io_uring_get_sqe(ring);
    if (sqe == nullptr) [[unlikely]] {
        __builtin_trap();
    }
    io_uring_prep_recv(sqe, conn_sock, recv_op->buffer, recv_op->len, 0);
    io_uring_sqe_set_data(sqe, recv_op);
}

int main() {
    auto listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    auto const l_address_in = sockaddr_in{
        .sin_family = AF_INET,
        .sin_port = htons(8585),
        .sin_addr = INADDR_ANY,
    };
    auto l_addr4bind = reinterpret_cast<sockaddr const*>(__builtin_addressof(l_address_in));
    auto l_bres = bind(listen_sock, l_addr4bind, sizeof(l_address_in));
    if (l_bres < 0) [[unlikely]] {
        __builtin_trap();
    }
    auto l_lres = listen(listen_sock, 16);
    if (l_lres < 0) [[unlikely]] {
        __builtin_trap();
    }

    OpType op_type{OpType::accept};
    io_uring ring;
    io_uring_queue_init(256, &ring, 0);
    // no longer get a change to release resource by using io_uring_queue_exit

    queue_accept(&ring, listen_sock);

    while (true) {
        io_uring_submit(&ring);

        io_uring_cqe* cqe;
        io_uring_wait_cqe(&ring, &cqe);

        switch (op_type) {
        case OpType::accept: {
            AcceptOp* accept_op = (AcceptOp*)io_uring_cqe_get_data(cqe);
            int conn_sock = cqe->res;
            io_uring_cqe_seen(&ring, cqe);

            if (conn_sock < 0) {
                fprintf(stderr, "accept %d: %s\n", -conn_sock, strerror(-conn_sock));
                free(accept_op);
                queue_accept(&ring, listen_sock);
                continue;
            }

            queue_recv(&ring, conn_sock);
            op_type = OpType::recv;

            free(accept_op);
        } break;
        case OpType::recv: {
            RecvOp* recv_op = (RecvOp*)io_uring_cqe_get_data(cqe);
            int res = cqe->res;
            io_uring_cqe_seen(&ring, cqe);

            if (res < 0) {
                // no need to release resources
                __builtin_trap();
            }
            else if (res > 0) {
                write(recv_op->conn_sock, recv_op->buffer, res);
                queue_recv(&ring, recv_op->conn_sock);
                ::std::destroy_at(recv_op);
                free(recv_op);
                continue;
            }

            close(recv_op->conn_sock);
            ::std::destroy_at(recv_op);
            free(recv_op);
            op_type = OpType::accept;
            queue_accept(&ring, listen_sock);
        } break;
        case OpType::send:
            [[fallthrough]];
        default:
            [[unlikely]] {
                __builtin_trap();
            }
        }
    }

    return 0;
}
