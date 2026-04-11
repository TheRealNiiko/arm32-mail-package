#ifndef NETWORK_AMD64_H
#define NETWORK_AMD64_H

#include "pthread.h"
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define NETWORK_BACKLOG 128
#define NETWORK_TIMEOUT 300
#define NETWORK_BUFFER_SIZE 16384

typedef struct {
    int socket_fd;
    struct sockaddr_in addr;
    time_t connected_at;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    pthread_mutex_t lock;
} network_connection_amd64_t;

typedef struct {
    int server_socket;
    struct sockaddr_in bind_addr;
    int backlog;
    volatile int listening;
    pthread_mutex_t lock;
} network_listener_amd64_t;

network_listener_amd64_t *network_create_listener_amd64(const char *bind_ip, uint16_t port, int backlog);
void network_free_listener_amd64(network_listener_amd64_t *listener);

int network_start_listening_amd64(network_listener_amd64_t *listener);
network_connection_amd64_t *network_accept_connection_amd64(network_listener_amd64_t *listener);

ssize_t network_send_amd64(network_connection_amd64_t *conn, const void *data, size_t len);
ssize_t network_receive_amd64(network_connection_amd64_t *conn, void *buffer, size_t len);

int network_set_keepalive_amd64(network_connection_amd64_t *conn, int keeper);
int network_set_timeout_amd64(network_connection_amd64_t *conn, int timeout_seconds);

#endif
