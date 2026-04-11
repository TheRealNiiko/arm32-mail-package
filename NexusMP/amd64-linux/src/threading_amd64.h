#ifndef THREADING_AMD64_H
#define THREADING_AMD64_H

#include "pthread.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct message_ref_amd64 {
    char references[2048];
    uint32_t uid;
    time_t date;
} message_ref_amd64_t;

typedef struct thread_node_amd64 {
    uint32_t uid;
    struct thread_node_amd64 *parent;
    struct thread_node_amd64 **children;
    int child_count;
    int child_capacity;
} thread_node_amd64_t;

typedef struct {
    char mailbox_name[256];
    message_ref_amd64_t *messages;
    int message_count;
    uint64_t modseq;
    pthread_rwlock_t lock;
} message_thread_amd64_t;

thread_node_amd64_t *thread_create_node_amd64(uint32_t uid);
void thread_free_node_amd64(thread_node_amd64_t *node);
int thread_add_child_amd64(thread_node_amd64_t *parent, thread_node_amd64_t *child);

thread_node_amd64_t *thread_build_tree_amd64(message_thread_amd64_t *mailbox);
char *thread_format_response_amd64(thread_node_amd64_t *root);

#endif
