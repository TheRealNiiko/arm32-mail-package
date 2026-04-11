#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include "pthread.h"
#include <stdint.h>
#include <time.h>
#include <sys/socket.h>

typedef struct {
    int sock;
    int authenticated;
    char username[256];
} client_t;

#define NOTIF_EXISTS 1
#define NOTIF_RECENT 2
#define NOTIF_EXPUNGE 3
#define NOTIF_FLAGS 4
#define NOTIF_QUOTA_EXCEEDED 5

typedef int notification_type_t;

typedef struct {
    notification_type_t type;
    uint32_t uid;
    time_t timestamp;
    char data[256];
} notification_t;

typedef struct {
    notification_t *notifications;
    unsigned int count;
    unsigned int capacity;
    unsigned int write_pos;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} notification_queue_t;

notification_queue_t *notif_create_queue(void);
void notif_free_queue(notification_queue_t *queue);

int notif_enqueue(notification_queue_t *queue, notification_type_t type, 
                 uint32_t uid, const char *data);
int notif_dequeue(notification_queue_t *queue, notification_t *notif);
int notif_wait_for_event(notification_queue_t *queue, int timeout_sec);

char *notif_format_exists(uint32_t count);
char *notif_format_recent(uint32_t count);
char *notif_format_expunge(uint32_t uid);
char *notif_format_flags(uint32_t uid, uint64_t flags);
char *notif_format_quota_exceeded(void);

int notif_send_idle_response(int client_sock, notification_t *notif);

#define FLAG_SEEN 0x01
#define FLAG_ANSWERED 0x02
#define FLAG_FLAGGED 0x04
#define FLAG_DELETED 0x08
#define FLAG_DRAFT 0x10
#define FLAG_RECENT 0x20

#endif
