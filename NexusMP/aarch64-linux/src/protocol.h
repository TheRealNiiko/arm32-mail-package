#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    int sock;
    int authenticated;
    char username[256];
    int idle_mode;
    time_t idle_start;
} client_t;

typedef struct {
    uint32_t uid;
    uint32_t seq;
    char subject[512];
    char from[256];
    char to[256];
    char cc[256];
    char bcc[256];
    time_t date;
    size_t size;
    uint64_t flags;
    int reference_count;
    char references[2048];
} message_t;

typedef struct {
    char name[256];
    message_t *messages;
    uint32_t message_count;
    uint32_t recent_count;
} mailbox_t;

int protocol_execute_command(client_t *client, const char *tag, const char *command,
                            const char *args, mailbox_t *mailbox);

#endif
