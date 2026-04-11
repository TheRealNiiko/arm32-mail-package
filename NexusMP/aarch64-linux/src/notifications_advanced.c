#include "notifications.h"

static pthread_rwlock_t notif_lock = PTHREAD_RWLOCK_INITIALIZER;

notification_queue_t *notif_create_queue(void) {
    notification_queue_t *queue = malloc(sizeof(notification_queue_t));
    if (!queue) return NULL;
    
    pthread_mutex_init(&queue->lock, NULL);
    pthread_cond_init(&queue->cond, NULL);
    
    queue->notifications = malloc(100 * sizeof(notification_t));
    if (!queue->notifications) {
        free(queue);
        return NULL;
    }
    
    queue->count = 0;
    queue->capacity = 100;
    queue->write_pos = 0;
    
    return queue;
}

void notif_free_queue(notification_queue_t *queue) {
    if (queue) {
        if (queue->notifications) free(queue->notifications);
        pthread_cond_destroy(&queue->cond);
        pthread_mutex_destroy(&queue->lock);
        free(queue);
    }
}

int notif_enqueue(notification_queue_t *queue, notification_type_t type, 
                 uint32_t uid, const char *data) {
    if (!queue) return -1;
    
    pthread_mutex_lock(&queue->lock);
    
    if (queue->count >= queue->capacity) {
        queue->capacity *= 2;
        notification_t *new_notifs = realloc(queue->notifications, 
                                            queue->capacity * sizeof(notification_t));
        if (!new_notifs) {
            pthread_mutex_unlock(&queue->lock);
            return -1;
        }
        queue->notifications = new_notifs;
    }
    
    queue->notifications[queue->write_pos].type = type;
    queue->notifications[queue->write_pos].uid = uid;
    queue->notifications[queue->write_pos].timestamp = time(NULL);
    
    if (data) {
        strncpy(queue->notifications[queue->write_pos].data, data, 255);
    } else {
        queue->notifications[queue->write_pos].data[0] = '\0';
    }
    
    queue->write_pos = (queue->write_pos + 1) % queue->capacity;
    queue->count++;
    
    if (queue->count == 1) {
        pthread_cond_broadcast(&queue->cond);
    }
    
    pthread_mutex_unlock(&queue->lock);
    return 0;
}

int notif_dequeue(notification_queue_t *queue, notification_t *notif) {
    if (!queue || !notif) return -1;
    
    pthread_mutex_lock(&queue->lock);
    
    if (queue->count == 0) {
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }
    
    unsigned int read_pos = (queue->write_pos - queue->count + queue->capacity) % queue->capacity;
    *notif = queue->notifications[read_pos];
    queue->count--;
    
    pthread_mutex_unlock(&queue->lock);
    return 0;
}

int notif_wait_for_event(notification_queue_t *queue, int timeout_sec) {
    struct timespec ts;
    int ret;
    
    if (!queue) return -1;
    
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_sec;
    
    pthread_mutex_lock(&queue->lock);
    
    if (queue->count > 0) {
        pthread_mutex_unlock(&queue->lock);
        return 1;
    }
    
    ret = pthread_cond_timedwait(&queue->cond, &queue->lock, &ts);
    
    pthread_mutex_unlock(&queue->lock);
    
    return (ret == 0) ? 1 : 0;
}

char *notif_format_exists(uint32_t count) {
    char *response;
    asprintf(&response, "* %u EXISTS\r\n", count);
    return response;
}

char *notif_format_recent(uint32_t count) {
    char *response;
    asprintf(&response, "* %u RECENT\r\n", count);
    return response;
}

char *notif_format_expunge(uint32_t uid) {
    char *response;
    asprintf(&response, "* %u EXPUNGE\r\n", uid);
    return response;
}

char *notif_format_flags(uint32_t uid, uint64_t flags) {
    char *response;
    char flags_str[256] = "(";
    
    if (flags & FLAG_SEEN) strcat(flags_str, "\\Seen ");
    if (flags & FLAG_ANSWERED) strcat(flags_str, "\\Answered ");
    if (flags & FLAG_FLAGGED) strcat(flags_str, "\\Flagged ");
    if (flags & FLAG_DELETED) strcat(flags_str, "\\Deleted ");
    if (flags & FLAG_DRAFT) strcat(flags_str, "\\Draft ");
    if (flags & FLAG_RECENT) strcat(flags_str, "\\Recent ");
    
    if (flags_str[1] != '\0') {
        flags_str[strlen(flags_str) - 1] = ')';
    } else {
        strcat(flags_str, ")");
    }
    
    asprintf(&response, "* %u FETCH (%s)\r\n", uid, flags_str);
    return response;
}

char *notif_format_quota_exceeded(void) {
    char *response;
    asprintf(&response, "* ALERT QUOTA exceeded\r\n");
    return response;
}

int notif_send_idle_response(int client_sock, notification_t *notif) {
    char *response = NULL;
    int sent = -1;
    
    if (!notif || client_sock < 0) return -1;
    
    switch (notif->type) {
        case NOTIF_EXISTS:
            response = notif_format_exists(notif->uid);
            break;
        case NOTIF_RECENT:
            response = notif_format_recent(notif->uid);
            break;
        case NOTIF_EXPUNGE:
            response = notif_format_expunge(notif->uid);
            break;
        case NOTIF_FLAGS:
            response = notif_format_flags(notif->uid, (uint64_t)(uintptr_t)notif->data);
            break;
        case NOTIF_QUOTA_EXCEEDED:
            response = notif_format_quota_exceeded();
            break;
        default:
            return -1;
    }
    
    if (response) {
        sent = send(client_sock, response, strlen(response), MSG_NOSIGNAL);
        free(response);
    }
    
    return (sent > 0) ? 0 : -1;
}
