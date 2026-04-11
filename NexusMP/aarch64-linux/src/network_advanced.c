#include "network.h"

int network_create_server_socket(int port, int backlog, int use_ipv6) {
    int server_sock;
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;
    int opt = 1;
    
    if (use_ipv6) {
        server_sock = socket(AF_INET6, SOCK_STREAM, 0);
    } else {
        server_sock = socket(AF_INET, SOCK_STREAM, 0);
    }
    
    if (server_sock < 0) {
        return -1;
    }
    
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(server_sock);
        return -1;
    }
    
    if (use_ipv6) {
        memset(&addr6, 0, sizeof(addr6));
        addr6.sin6_family = AF_INET6;
        addr6.sin6_addr = in6addr_any;
        addr6.sin6_port = htons(port);
        
        if (bind(server_sock, (struct sockaddr *)&addr6, sizeof(addr6)) < 0) {
            close(server_sock);
            return -1;
        }
    } else {
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port);
        
        if (bind(server_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            close(server_sock);
            return -1;
        }
    }
    
    if (listen(server_sock, backlog) < 0) {
        close(server_sock);
        return -1;
    }
    
    return server_sock;
}

int network_set_socket_options(int sock) {
    int opt = 1;
    struct linger linger;
    
    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) < 0) {
        return -1;
    }
    
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
        return -1;
    }
    
    linger.l_onoff = 1;
    linger.l_linger = 30;
    
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) < 0) {
        return -1;
    }
    
    return 0;
}

int network_set_nonblocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) return -1;
    
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        return -1;
    }
    
    return 0;
}

int network_set_read_timeout(int sock, int timeout_sec) {
    struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        return -1;
    }
    
    return 0;
}

int network_set_write_timeout(int sock, int timeout_sec) {
    struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
        return -1;
    }
    
    return 0;
}

int network_send_data(int sock, const char *data, size_t size) {
    ssize_t sent = 0;
    ssize_t total = 0;
    
    while (total < size) {
        sent = send(sock, data + total, size - total, MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            return -1;
        }
        if (sent == 0) {
            break;
        }
        total += sent;
    }
    
    return (total == size) ? 0 : -1;
}

int network_receive_data(int sock, char *buffer, size_t buffer_size, int timeout) {
    fd_set readfds;
    struct timeval tv;
    ssize_t received;
    
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
    
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    
    int activity = select(sock + 1, &readfds, NULL, NULL, &tv);
    
    if (activity <= 0) {
        return -1;
    }
    
    if (!FD_ISSET(sock, &readfds)) {
        return -1;
    }
    
    received = recv(sock, buffer, buffer_size - 1, 0);
    
    if (received > 0) {
        buffer[received] = '\0';
        return received;
    }
    
    return -1;
}

int network_create_epoll(void) {
    return epoll_create1(EPOLL_CLOEXEC);
}

int network_add_to_epoll(int epoll_fd, int sock, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = sock;
    
    return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &ev);
}

int network_remove_from_epoll(int epoll_fd, int sock) {
    return epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock, NULL);
}

int network_wait_events(int epoll_fd, struct epoll_event *events, int max_events, int timeout) {
    return epoll_wait(epoll_fd, events, max_events, timeout);
}

int network_setup_keepalive(int sock) {
    int opt = 1;
    int keepidle = 60;
    int keepintvl = 10;
    int keepcnt = 5;
    
    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) < 0) {
        return -1;
    }
    
    if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle)) < 0) {
        return -1;
    }
    
    if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl)) < 0) {
        return -1;
    }
    
    if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt)) < 0) {
        return -1;
    }
    
    return 0;
}

int network_check_connection_alive(int sock) {
    int error = 0;
    socklen_t len = sizeof(error);
    
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
        return -1;
    }
    
    if (error != 0) {
        return -1;
    }
    
    return 0;
}
