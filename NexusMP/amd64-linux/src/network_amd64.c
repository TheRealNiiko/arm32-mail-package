#include "network_amd64.h"
#include <errno.h>
#include <string.h>

network_listener_amd64_t *network_create_listener_amd64(const char *bind_ip, uint16_t port, int backlog) {
    if (!bind_ip) return NULL;

    network_listener_amd64_t *listener = malloc(sizeof(network_listener_amd64_t));
    if (!listener) return NULL;

    listener->server_socket = -1;
    listener->backlog = backlog;
    listener->listening = 0;
    pthread_mutex_init(&listener->lock, NULL);

    memset(&listener->bind_addr, 0, sizeof(listener->bind_addr));
    listener->bind_addr.sin_family = AF_INET;
    listener->bind_addr.sin_port = htons(port);
    inet_pton(AF_INET, bind_ip, &listener->bind_addr.sin_addr);

    return listener;
}

void network_free_listener_amd64(network_listener_amd64_t *listener) {
    if (!listener) return;

    if (listener->server_socket >= 0) {
        close(listener->server_socket);
    }

    pthread_mutex_destroy(&listener->lock);
    free(listener);
}

int network_start_listening_amd64(network_listener_amd64_t *listener) {
    if (!listener) return -1;

    pthread_mutex_lock(&listener->lock);

    listener->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listener->server_socket < 0) {
        perror("socket");
        pthread_mutex_unlock(&listener->lock);
        return -1;
    }

    int opt = 1;
    if (setsockopt(listener->server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listener->server_socket);
        listener->server_socket = -1;
        pthread_mutex_unlock(&listener->lock);
        return -1;
    }

    if (bind(listener->server_socket, (struct sockaddr *)&listener->bind_addr, sizeof(listener->bind_addr)) < 0) {
        perror("bind");
        close(listener->server_socket);
        listener->server_socket = -1;
        pthread_mutex_unlock(&listener->lock);
        return -1;
    }

    if (listen(listener->server_socket, listener->backlog) < 0) {
        perror("listen");
        close(listener->server_socket);
        listener->server_socket = -1;
        pthread_mutex_unlock(&listener->lock);
        return -1;
    }

    listener->listening = 1;
    pthread_mutex_unlock(&listener->lock);

    return 0;
}

network_connection_amd64_t *network_accept_connection_amd64(network_listener_amd64_t *listener) {
    if (!listener || listener->server_socket < 0) return NULL;

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    int socket_fd = accept(listener->server_socket, (struct sockaddr *)&client_addr, &addr_len);
    if (socket_fd < 0) {
        perror("accept");
        return NULL;
    }

    network_connection_amd64_t *conn = malloc(sizeof(network_connection_amd64_t));
    if (!conn) {
        close(socket_fd);
        return NULL;
    }

    conn->socket_fd = socket_fd;
    memcpy(&conn->addr, &client_addr, sizeof(client_addr));
    conn->connected_at = time(NULL);
    conn->bytes_sent = 0;
    conn->bytes_received = 0;
    pthread_mutex_init(&conn->lock, NULL);

    return conn;
}

ssize_t network_send_amd64(network_connection_amd64_t *conn, const void *data, size_t len) {
    if (!conn || conn->socket_fd < 0 || !data) return -1;

    pthread_mutex_lock(&conn->lock);

    ssize_t n = send(conn->socket_fd, data, len, 0);
    if (n > 0) {
        conn->bytes_sent += n;
    }

    pthread_mutex_unlock(&conn->lock);

    return n;
}

ssize_t network_receive_amd64(network_connection_amd64_t *conn, void *buffer, size_t len) {
    if (!conn || conn->socket_fd < 0 || !buffer) return -1;

    pthread_mutex_lock(&conn->lock);

    ssize_t n = recv(conn->socket_fd, buffer, len, 0);
    if (n > 0) {
        conn->bytes_received += n;
    }

    pthread_mutex_unlock(&conn->lock);

    return n;
}

int network_set_keepalive_amd64(network_connection_amd64_t *conn, int keeper) {
    if (!conn || conn->socket_fd < 0) return -1;

    return setsockopt(conn->socket_fd, SOL_SOCKET, SO_KEEPALIVE, &keeper, sizeof(keeper));
}

int network_set_timeout_amd64(network_connection_amd64_t *conn, int timeout_seconds) {
    if (!conn || conn->socket_fd < 0) return -1;

    struct timeval tv;
    tv.tv_sec = timeout_seconds;
    tv.tv_usec = 0;

    return setsockopt(conn->socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}
