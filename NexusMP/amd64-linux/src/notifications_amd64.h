#ifndef NOTIFICATIONS_AMD64_H
#define NOTIFICATIONS_AMD64_H

#include "pthread.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

typedef enum {
    NOTIFY_EXISTS,
    NOTIFY_EXPUNGE,
    NOTIFY_FLAGS,
    NOTIFY_RECENT
} notify_type_amd64_t;

typedef struct {
    notify_type_amd64_t type;
    uint32_t uid;
    uint32_t count;
    char *flags;
} notification_amd64_t;

typedef struct {
    int client_socket;
    char mailbox_name[256];
    int idle_active;
    pthread_mutex_t lock;
    pthread_cond_t condition;
} idle_session_amd64_t;

idle_session_amd64_t *idle_create_session_amd64(int socket_fd, const char *mailbox);
void idle_free_session_amd64(idle_session_amd64_t *session);

int idle_send_notification_amd64(idle_session_amd64_t *session, notification_amd64_t *notif);
int idle_push_message_exists_amd64(idle_session_amd64_t *session, uint32_t count);
int idle_push_message_expunge_amd64(idle_session_amd64_t *session, uint32_t uid);
int idle_push_flags_amd64(idle_session_amd64_t *session, uint32_t uid, const char *flags);

#endif
