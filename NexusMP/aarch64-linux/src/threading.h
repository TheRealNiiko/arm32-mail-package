#ifndef THREADING_H
#define THREADING_H

#include "pthread.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct thread_node {
    uint32_t uid;
    struct thread_node *parent;
    struct thread_node **children;
    unsigned int child_count;
    unsigned int child_capacity;
} thread_node_t;

typedef struct message {
    uint32_t uid;
    char references[2048];
    int reference_count;
} message_t;

typedef struct {
    char name[256];
    message_t *messages;
    uint32_t message_count;
    uint64_t modseq;
    pthread_rwlock_t lock;
} mailbox_t;

thread_node_t *thread_create_node(uint32_t uid);
void thread_free_node(thread_node_t *node);
int thread_add_child(thread_node_t *parent, thread_node_t *child);

thread_node_t *thread_parse_references(const char *references, mailbox_t *mailbox);
int thread_build_tree(mailbox_t *mailbox, thread_node_t **root);

char *thread_format_response(thread_node_t *root);
int thread_update_modseq(thread_node_t *node, uint64_t *modseq);

#endif
