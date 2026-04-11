#include "notifications_amd64.h"
#include <stdio.h>

idle_session_amd64_t *idle_create_session_amd64(int socket_fd, const char *mailbox) {
    if (socket_fd < 0 || !mailbox) return NULL;

    idle_session_amd64_t *session = malloc(sizeof(idle_session_amd64_t));
    if (!session) return NULL;

    session->client_socket = socket_fd;
    strncpy(session->mailbox_name, mailbox, 255);
    session->mailbox_name[255] = '\0';
    session->idle_active = 0;
    pthread_mutex_init(&session->lock, NULL);
    pthread_cond_init(&session->condition, NULL);

    return session;
}

void idle_free_session_amd64(idle_session_amd64_t *session) {
    if (!session) return;

    pthread_mutex_destroy(&session->lock);
    pthread_cond_destroy(&session->condition);
    free(session);
}

int idle_send_notification_amd64(idle_session_amd64_t *session, notification_amd64_t *notif) {
    if (!session || !notif) return -1;

    pthread_mutex_lock(&session->lock);

    if (!session->idle_active) {
        pthread_mutex_unlock(&session->lock);
        return -1;
    }

    char message[256];
    switch (notif->type) {
        case NOTIFY_EXISTS:
            snprintf(message, sizeof(message), "* %u EXISTS\r\n", notif->count);
            break;
        case NOTIFY_EXPUNGE:
            snprintf(message, sizeof(message), "* %u EXPUNGE\r\n", notif->uid);
            break;
        case NOTIFY_FLAGS:
            snprintf(message, sizeof(message), "* %u FETCH (FLAGS (%s))\r\n", notif->uid, notif->flags);
            break;
        case NOTIFY_RECENT:
            snprintf(message, sizeof(message), "* %u RECENT\r\n", notif->count);
            break;
        default:
            pthread_mutex_unlock(&session->lock);
            return -1;
    }

    send(session->client_socket, message, strlen(message), 0);
    pthread_mutex_unlock(&session->lock);

    return 0;
}

int idle_push_message_exists_amd64(idle_session_amd64_t *session, uint32_t count) {
    if (!session) return -1;

    notification_amd64_t notif;
    notif.type = NOTIFY_EXISTS;
    notif.count = count;
    notif.uid = 0;
    notif.flags = NULL;

    return idle_send_notification_amd64(session, &notif);
}

int idle_push_message_expunge_amd64(idle_session_amd64_t *session, uint32_t uid) {
    if (!session) return -1;

    notification_amd64_t notif;
    notif.type = NOTIFY_EXPUNGE;
    notif.uid = uid;
    notif.count = 0;
    notif.flags = NULL;

    return idle_send_notification_amd64(session, &notif);
}

int idle_push_flags_amd64(idle_session_amd64_t *session, uint32_t uid, const char *flags) {
    if (!session || !flags) return -1;

    notification_amd64_t notif;
    notif.type = NOTIFY_FLAGS;
    notif.uid = uid;
    notif.count = 0;
    notif.flags = (char *)flags;

    return idle_send_notification_amd64(session, &notif);
}
