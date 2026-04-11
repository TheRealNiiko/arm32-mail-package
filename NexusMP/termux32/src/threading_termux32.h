#ifndef THREADING_TERMUX32_H
#define THREADING_TERMUX32_H

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ARM32: halved reference string and mailbox name */
typedef struct message_ref_t32 {
    char     references[1024];
    uint32_t uid;
    time_t   date;
} message_ref_t32_t;

typedef struct thread_node_t32 {
    uint32_t uid;
    struct thread_node_t32  *parent;
    struct thread_node_t32 **children;
    int child_count;
    int child_capacity;
} thread_node_t32_t;

typedef struct {
    char               mailbox_name[128];   /* ARM32: halved from 256 */
    message_ref_t32_t *messages;
    int                message_count;
    uint32_t           modseq;
    pthread_rwlock_t   lock;
} message_thread_t32_t;

thread_node_t32_t *thread_create_node_t32(uint32_t uid);
void thread_free_node_t32(thread_node_t32_t *node);
int  thread_add_child_t32(thread_node_t32_t *parent, thread_node_t32_t *child);

thread_node_t32_t *thread_build_tree_t32(message_thread_t32_t *mailbox);
char *thread_format_response_t32(thread_node_t32_t *root);

#endif
