#ifndef SEARCH_H
#define SEARCH_H

#include "pthread.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    uint32_t *uids;
    uint32_t uid_count;
    uint32_t uid_capacity;
} search_result_t;

typedef struct message {
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
    uint32_t message_capacity;
    uint32_t recent_count;
    uint64_t modseq;
    pthread_rwlock_t lock;
} mailbox_t;

search_result_t *search_create_result(void);
void search_free_result(search_result_t *result);
void search_add_uid(search_result_t *result, uint32_t uid);

int search_all_messages(mailbox_t *mailbox, search_result_t *result);
int search_by_flags(mailbox_t *mailbox, uint64_t flags, int match_all, search_result_t *result);
int search_by_date_range(mailbox_t *mailbox, time_t start, time_t end, search_result_t *result);
int search_by_text(mailbox_t *mailbox, const char *text, const char *field, search_result_t *result);
int search_by_regex(mailbox_t *mailbox, const char *pattern, search_result_t *result);

char *format_search_result(search_result_t *result);

#endif
