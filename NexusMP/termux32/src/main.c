#include "nexus_mp_termux32.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

static nexus_mp_server_t *global_server = NULL;

void signal_handler(int sig) {
    if ((sig == SIGTERM || sig == SIGINT) && global_server)
        nexus_mp_stop_server(global_server);
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    /* Root is required: binding to privileged port 143 and managing
     * system mail spool directories demands it. */
    if (getuid() != 0 || geteuid() != 0) {
        fprintf(stderr, "nexus-mp: must be run as root\n");
        return 1;
    }

    uint16_t port       = NEXUS_MP_PORT;
    int      num_threads = 4;   /* ARM32: 4 threads — conservative for mobile CPU */

    printf("NexusMP Termux32 Server v%s\n", NEXUS_MP_VERSION);
    printf("Initializing server on port %u with %d threads...\n", port, num_threads);

    global_server = nexus_mp_create_server(port, num_threads);
    if (!global_server) {
        fprintf(stderr, "Failed to create server\n");
        return 1;
    }

    signal(SIGTERM, signal_handler);
    signal(SIGINT,  signal_handler);

    if (nexus_mp_start_server(global_server) < 0) {
        fprintf(stderr, "Failed to start server\n");
        nexus_mp_free_server(global_server);
        return 1;
    }

    printf("Server started. Press Ctrl+C to stop.\n");

    while (global_server->running)
        sleep(1);

    printf("Shutting down...\n");
    nexus_mp_free_server(global_server);

    return 0;
}
