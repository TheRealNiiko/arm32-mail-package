#ifndef NEXUS_MP_TERMUX32_H
#define NEXUS_MP_TERMUX32_H

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define NEXUS_MP_PORT          143
#define NEXUS_MP_VERSION       "1.0.0-termux32"
#define MAX_CLIENTS            64                   /* ARM32: limited address space */
#define MAX_MAILBOX_NAME       128                  /* ARM32: halved vs amd64 */
#define MAX_MESSAGE_SIZE       (16 * 1024 * 1024)   /* ARM32: 16MB ceiling */
#define COMMAND_BUFFER_SIZE    4096                 /* ARM32: halved vs amd64 */

/* All counters fit in 32 bits on this arch — no uint64_t */
typedef struct client_connection {
    int          socket_fd;
    int          authenticated;
    uint32_t     client_id;
    char         username[64];
    char         selected_mailbox[MAX_MAILBOX_NAME];
    time_t       last_activity;
    pthread_mutex_t lock;
} client_connection_t;

typedef struct {
    int              listen_socket;
    uint16_t         port;
    uint32_t         total_clients;
    pthread_t       *thread_pool;
    int              num_threads;
    volatile int     running;
} nexus_mp_server_t;

nexus_mp_server_t *nexus_mp_create_server(uint16_t port, int num_threads);
int  nexus_mp_start_server(nexus_mp_server_t *server);
void nexus_mp_stop_server(nexus_mp_server_t *server);
void nexus_mp_free_server(nexus_mp_server_t *server);

int   nexus_mp_handle_client(client_connection_t *client);
void  nexus_mp_send_response(int sock_fd, const char *response);
char *nexus_mp_read_command(int sock_fd);

#endif
