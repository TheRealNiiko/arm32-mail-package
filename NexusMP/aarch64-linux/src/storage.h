#ifndef STORAGE_H
#define STORAGE_H

#include "pthread.h"
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

typedef enum {
    STORAGE_MAILDIR = 1,
    STORAGE_MBOX = 2,
    STORAGE_DBOX = 3
} storage_backend_t;

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
    char *raw_data;
} message_t;

typedef struct {
    char name[256];
    message_t *messages;
    uint32_t message_count;
    uint32_t message_capacity;
    uint32_t recent_count;
    uint64_t modseq;
    pthread_rwlock_t lock;
} mailbox_t;

int storage_init(const char *base_path, storage_backend_t backend);
int storage_cleanup(void);

int maildir_init(const char *username, const char *maildir_path);
mailbox_t *storage_get_mailbox(const char *username, const char *mailbox_name);
message_t *storage_get_message(mailbox_t *mailbox, uint32_t uid);

int maildir_list_messages(const char *maildir_path, message_t **messages, uint32_t *count);
int maildir_add_message(const char *maildir_path, const char *message_data, uint32_t size, uint32_t *new_uid);
int maildir_delete_message(const char *maildir_path, uint32_t uid);
int maildir_expunge_messages(const char *maildir_path);
int maildir_set_flags(const char *maildir_path, uint32_t uid, uint64_t flags);
uint64_t maildir_get_flags(const char *maildir_path, uint32_t uid);
int maildir_copy_message(const char *src_path, const char *dst_path, uint32_t uid);
int maildir_create_mailbox(const char *maildir_path, const char *mailbox_name);
int maildir_delete_mailbox(const char *maildir_path, const char *mailbox_name);
int maildir_rename_mailbox(const char *maildir_path, const char *old_name, const char *new_name);

int read_message_headers(const char *filename, message_t *msg);
uint32_t storage_find_uid_by_message_id(mailbox_t *mailbox, const char *msg_id);

#endif
