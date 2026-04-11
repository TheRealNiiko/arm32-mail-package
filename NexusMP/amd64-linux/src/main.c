#include "nexus_mp_amd64.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

static nexus_mp_server_t *global_server = NULL;

void signal_handler(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        if (global_server) {
            nexus_mp_stop_server(global_server);
        }
    }
}

int main(int argc, char *argv[]) {
    uint16_t port = NEXUS_MP_PORT;
    int num_threads = 16;

    printf("NexusMP AMD64 Server v%s\n", NEXUS_MP_VERSION);
    printf("Initializing server on port %u with %d threads...\n", port, num_threads);

    global_server = nexus_mp_create_server(port, num_threads);
    if (!global_server) {
        fprintf(stderr, "Failed to create server\n");
        return 1;
    }

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    if (nexus_mp_start_server(global_server) < 0) {
        fprintf(stderr, "Failed to start server\n");
        nexus_mp_free_server(global_server);
        return 1;
    }

    printf("Server started successfully. Press Ctrl+C to stop.\n");

    while (global_server->running) {
        sleep(1);
    }

    printf("Shutting down...\n");
    nexus_mp_free_server(global_server);

    return 0;
}
