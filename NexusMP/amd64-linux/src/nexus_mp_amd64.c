#include "nexus_mp_amd64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

nexus_mp_server_t *nexus_mp_create_server(uint16_t port, int num_threads) {
    nexus_mp_server_t *server = malloc(sizeof(nexus_mp_server_t));
    if (!server) {
        perror("malloc");
        return NULL;
    }

    server->listen_socket = -1;
    server->port = port;
    server->total_clients = 0;
    server->num_threads = num_threads;
    server->running = 0;
    
    server->thread_pool = malloc(sizeof(pthread_t) * num_threads);
    if (!server->thread_pool) {
        perror("malloc");
        free(server);
        return NULL;
    }

    return server;
}

void nexus_mp_free_server(nexus_mp_server_t *server) {
    if (!server) return;
    
    if (server->listen_socket >= 0) {
        close(server->listen_socket);
    }
    if (server->thread_pool) {
        free(server->thread_pool);
    }
    free(server);
}

int nexus_mp_start_server(nexus_mp_server_t *server) {
    if (!server || server->listen_socket >= 0) {
        return -1;
    }

    server->listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->listen_socket < 0) {
        perror("socket");
        return -1;
    }

    int opt = 1;
    if (setsockopt(server->listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server->listen_socket);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(server->port);

    if (bind(server->listen_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server->listen_socket);
        return -1;
    }

    if (listen(server->listen_socket, 128) < 0) {
        perror("listen");
        close(server->listen_socket);
        return -1;
    }

    server->running = 1;

    printf("[%s] NexusMP AMD64 server listening on port %u\n", NEXUS_MP_VERSION, server->port);

    return 0;
}

void nexus_mp_stop_server(nexus_mp_server_t *server) {
    if (!server) return;
    server->running = 0;
}

void nexus_mp_send_response(int sock_fd, const char *response) {
    if (sock_fd < 0 || !response) return;
    send(sock_fd, response, strlen(response), 0);
}

char *nexus_mp_read_command(int sock_fd) {
    char *buffer = malloc(COMMAND_BUFFER_SIZE);
    if (!buffer) return NULL;

    ssize_t n = recv(sock_fd, buffer, COMMAND_BUFFER_SIZE - 1, 0);
    if (n <= 0) {
        free(buffer);
        return NULL;
    }

    buffer[n] = '\0';
    return buffer;
}

int nexus_mp_handle_client(client_connection_t *client) {
    if (!client || client->socket_fd < 0) {
        return -1;
    }

    nexus_mp_send_response(client->socket_fd, "* OK [CAPABILITY IMAP4rev1 IDLE STARTTLS COMPRESS=DEFLATE] NexusMP AMD64 ready\r\n");

    while (1) {
        char *cmd = nexus_mp_read_command(client->socket_fd);
        if (!cmd) break;

        if (strstr(cmd, "CAPABILITY")) {
            nexus_mp_send_response(client->socket_fd, "* CAPABILITY IMAP4rev1 IDLE STARTTLS COMPRESS=DEFLATE AUTH=PLAIN AUTH=CRAM-MD5\r\n");
            nexus_mp_send_response(client->socket_fd, "OK CAPABILITY completed\r\n");
        } else if (strstr(cmd, "LOGOUT")) {
            nexus_mp_send_response(client->socket_fd, "* BYE NexusMP AMD64 closing connection\r\n");
            nexus_mp_send_response(client->socket_fd, "OK LOGOUT completed\r\n");
            free(cmd);
            break;
        } else {
            nexus_mp_send_response(client->socket_fd, "OK Command processed\r\n");
        }

        free(cmd);
    }

    return 0;
}
