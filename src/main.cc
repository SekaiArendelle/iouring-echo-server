#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main() {
    auto l_sock = socket(AF_INET, SOCK_STREAM, 0);
    auto const l_address_in = sockaddr_in{
        .sin_family = AF_INET,
        .sin_port = htons(8585),
        .sin_addr = INADDR_ANY,
    };
    auto l_addr4bind = reinterpret_cast<sockaddr const*>(__builtin_addressof(l_address_in));
    auto l_bres = bind(l_sock, l_addr4bind, sizeof(l_address_in));
    if (l_bres < 0) [[unlikely]] {
        __builtin_trap();
    }
    auto l_lres = listen(l_sock, 16);
    if (l_lres < 0) [[unlikely]] {
        __builtin_trap();
    }

    while (true) {
        auto l_conn = accept(l_sock, nullptr, nullptr);
        constexpr size_t l_rbuf_len{128};
        char l_rbuf[l_rbuf_len]{};

        while (true) {
            ssize_t l_rres = read(l_conn, l_rbuf, l_rbuf_len);
            if (l_rres <= 0) {
                break;
            }
            write(l_conn, l_rbuf, l_rres);
        }
        close(l_conn);
    }

    return 0;
}
