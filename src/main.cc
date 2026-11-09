#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <liburing.h>

#include "op.hh"

static void queue_accept(io_uring* ring, int listen_fd) {
    Op* op = (Op*)malloc(sizeof(Op));
    if (op == nullptr) [[unlikely]] {
        __builtin_trap();
    }
    op->op_type = OpType::accept;
    op->fd = listen_fd;
    op->len = sizeof(op->buffer);

    io_uring_sqe* sqe = io_uring_get_sqe(ring);
    if (sqe == nullptr) [[unlikely]] {
        __builtin_trap();
    }

    io_uring_prep_accept(sqe, listen_fd, reinterpret_cast<sockaddr*>(__builtin_addressof(op->buffer)),
                         __builtin_addressof(op->len), 0);
    io_uring_sqe_set_data(sqe, op);
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

    io_uring ring;
    io_uring_queue_init(256, &ring, 0);
    // no longer get a change to release resource by using io_uring_queue_exit

    queue_accept(&ring, listen_sock);

    while (true) {
        io_uring_submit(&ring);

        io_uring_cqe* cqe;
        io_uring_wait_cqe(&ring, &cqe);

        Op* op = (Op*)io_uring_cqe_get_data(cqe);
        int conn_sock = cqe->res;
        io_uring_cqe_seen(&ring, cqe);

        if (op->op_type == OpType::accept) {
            if (conn_sock < 0) {
                fprintf(stderr, "accept %d: %s\n", -conn_sock, strerror(-conn_sock));
            }
            else {
                constexpr size_t l_rbuf_len{128};
                char l_rbuf[l_rbuf_len]{};
                while (true) {
                    ssize_t l_rres = read(conn_sock, l_rbuf, l_rbuf_len);
                    if (l_rres <= 0) {
                        break;
                    }
                    write(conn_sock, l_rbuf, l_rres);
                }
                close(conn_sock);
            }
            free(op);
            queue_accept(&ring, listen_sock);
        }
    }

    return 0;
}
