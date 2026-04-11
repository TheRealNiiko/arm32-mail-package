#ifndef STORAGE_AMD64_H
#define STORAGE_AMD64_H

#include "pthread.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#define MAILDIR_TEMP "tmp"
#define MAILDIR_NEW "new"
#define MAILDIR_CUR "cur"
#define MAX_UID_VALIDITY 0xFFFFFFFFUL

typedef struct {
    uint32_t uid;
    char filename[512];
    uint64_t size;
    time_t date;
    int seen;
    int flagged;
    int answered;
    int deleted;
    uint64_t modseq;
} message_metadata_amd64_t;

typedef struct {
    char maildir_path[512];
    message_metadata_amd64_t *messages;
    int message_count;
    int message_capacity;
    uint32_t next_uid;
    uint64_t uidvalidity;
    uint64_t modseq;
    pthread_rwlock_t lock;
} mailbox_storage_amd64_t;

mailbox_storage_amd64_t *storage_open_mailbox_amd64(const char *maildir_path);
void storage_close_mailbox_amd64(mailbox_storage_amd64_t *mailbox);

int storage_add_message_amd64(mailbox_storage_amd64_t *mailbox, const char *message_data, uint64_t size);
int storage_delete_message_amd64(mailbox_storage_amd64_t *mailbox, uint32_t uid);
int storage_mark_seen_amd64(mailbox_storage_amd64_t *mailbox, uint32_t uid);

message_metadata_amd64_t *storage_get_message_amd64(mailbox_storage_amd64_t *mailbox, uint32_t uid);
char *storage_fetch_message_amd64(mailbox_storage_amd64_t *mailbox, uint32_t uid);

int storage_sync_mailbox_amd64(mailbox_storage_amd64_t *mailbox);

#endif
