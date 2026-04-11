#ifndef NETWORK_TERMUX32_H
#define NETWORK_TERMUX32_H

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define NETWORK_BACKLOG      32      /* ARM32: mobile handles fewer concurrent clients */
#define NETWORK_TIMEOUT      180     /* ARM32: tighter idle timeout (3 min vs 5) */
#define NETWORK_BUFFER_SIZE  8192    /* ARM32: halved vs amd64's 16384 */

typedef struct {
    int              socket_fd;
    struct sockaddr_in addr;
    time_t           connected_at;
    uint32_t         bytes_sent;     /* ARM32: uint32_t — 4GB per connection is enough */
    uint32_t         bytes_received;
    pthread_mutex_t  lock;
} network_connection_t32_t;

typedef struct {
    int              server_socket;
    struct sockaddr_in bind_addr;
    int              backlog;
    volatile int     listening;
    pthread_mutex_t  lock;
} network_listener_t32_t;

network_listener_t32_t *network_create_listener_t32(const char *bind_ip, uint16_t port, int backlog);
void network_free_listener_t32(network_listener_t32_t *listener);

int network_start_listening_t32(network_listener_t32_t *listener);
network_connection_t32_t *network_accept_connection_t32(network_listener_t32_t *listener);

ssize_t network_send_t32(network_connection_t32_t *conn, const void *data, size_t len);
ssize_t network_receive_t32(network_connection_t32_t *conn, void *buffer, size_t len);

int network_set_keepalive_t32(network_connection_t32_t *conn, int keepalive);
int network_set_timeout_t32(network_connection_t32_t *conn, int timeout_seconds);

#endif
