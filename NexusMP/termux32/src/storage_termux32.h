#ifndef STORAGE_TERMUX32_H
#define STORAGE_TERMUX32_H

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#define MAILDIR_TEMP "tmp"
#define MAILDIR_NEW  "new"
#define MAILDIR_CUR  "cur"

/* ARM32: 32-bit UIDs and sizes — fits a 32-bit register, no padding waste */
typedef struct {
    uint32_t uid;
    char     filename[512];     /* ARM32: half of amd64's 1024 */
    uint32_t size;              /* ARM32: uint32_t — 4GB per message is plenty */
    time_t   date;
    int      seen;
    int      flagged;
    int      answered;
    int      deleted;
    uint32_t modseq;            /* ARM32: uint32_t sequence counter */
} message_metadata_t32_t;

typedef struct {
    char                  maildir_path[256];    /* ARM32: half of amd64's 512 */
    message_metadata_t32_t *messages;
    int                   message_count;
    int                   message_capacity;
    uint32_t              next_uid;
    uint32_t              uidvalidity;
    uint32_t              modseq;
    pthread_rwlock_t      lock;
} mailbox_storage_t32_t;

mailbox_storage_t32_t *storage_open_mailbox_t32(const char *maildir_path);
void storage_close_mailbox_t32(mailbox_storage_t32_t *mailbox);

int  storage_add_message_t32(mailbox_storage_t32_t *mailbox, const char *data, uint32_t size);
int  storage_delete_message_t32(mailbox_storage_t32_t *mailbox, uint32_t uid);
int  storage_mark_seen_t32(mailbox_storage_t32_t *mailbox, uint32_t uid);

message_metadata_t32_t *storage_get_message_t32(mailbox_storage_t32_t *mailbox, uint32_t uid);
char *storage_fetch_message_t32(mailbox_storage_t32_t *mailbox, uint32_t uid);

int storage_sync_mailbox_t32(mailbox_storage_t32_t *mailbox);

#endif
