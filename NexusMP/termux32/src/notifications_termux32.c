#include "notifications_termux32.h"
#include <stdio.h>

idle_session_t32_t *idle_create_session_t32(int socket_fd, const char *mailbox) {
    if (socket_fd < 0 || !mailbox) return NULL;

    idle_session_t32_t *session = malloc(sizeof(idle_session_t32_t));
    if (!session) return NULL;

    session->client_socket = socket_fd;
    strncpy(session->mailbox_name, mailbox, 127);
    session->mailbox_name[127] = '\0';
    session->idle_active = 0;
    pthread_mutex_init(&session->lock, NULL);
    pthread_cond_init(&session->condition, NULL);

    return session;
}

void idle_free_session_t32(idle_session_t32_t *session) {
    if (!session) return;

    pthread_mutex_destroy(&session->lock);
    pthread_cond_destroy(&session->condition);
    free(session);
}

int idle_send_notification_t32(idle_session_t32_t *session, notification_t32_t *notif) {
    if (!session || !notif) return -1;

    pthread_mutex_lock(&session->lock);

    if (!session->idle_active) {
        pthread_mutex_unlock(&session->lock);
        return -1;
    }

    /* ARM32: 128-byte stack message (was 256) */
    char message[128];
    switch (notif->type) {
        case NOTIFY_EXISTS:
            snprintf(message, sizeof(message), "* %u EXISTS\r\n", notif->count);
            break;
        case NOTIFY_EXPUNGE:
            snprintf(message, sizeof(message), "* %u EXPUNGE\r\n", notif->uid);
            break;
        case NOTIFY_FLAGS:
            snprintf(message, sizeof(message), "* %u FETCH (FLAGS (%s))\r\n",
                     notif->uid, notif->flags ? notif->flags : "");
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

int idle_push_message_exists_t32(idle_session_t32_t *session, uint32_t count) {
    if (!session) return -1;

    notification_t32_t notif = { NOTIFY_EXISTS, 0, count, NULL };
    return idle_send_notification_t32(session, &notif);
}

int idle_push_message_expunge_t32(idle_session_t32_t *session, uint32_t uid) {
    if (!session) return -1;

    notification_t32_t notif = { NOTIFY_EXPUNGE, uid, 0, NULL };
    return idle_send_notification_t32(session, &notif);
}

int idle_push_flags_t32(idle_session_t32_t *session, uint32_t uid, const char *flags) {
    if (!session || !flags) return -1;

    notification_t32_t notif = { NOTIFY_FLAGS, uid, 0, (char *)flags };
    return idle_send_notification_t32(session, &notif);
}
