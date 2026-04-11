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
#ifndef THREADING_AMD64_H\n#define THREADING_AMD64_H\n\n#include \"pthread.h\"\n#include <stdint.h>\n#include <stdlib.h>\n#include <string.h>\n\ntypedef struct thread_node_amd64 {\n    uint32_t uid;\n    struct thread_node_amd64 *parent;\n    struct thread_node_amd64 **children;\n    int child_count;\n    int child_capacity;\n} thread_node_amd64_t;\n\ntypedef struct {\n    char mailbox_name[256];\n    message_ref_amd64_t *messages;\n    int message_count;\n    uint64_t modseq;\n    pthread_rwlock_t lock;\n} message_thread_amd64_t;\n\ntyped _def struct {\n    char references[2048];\n    uint32_t uid;\n    time_t date;\n} message_ref_amd64_t;\n\nthread_node_amd64_t *thread_create_node_amd64(uint32_t uid);\nvoid thread_free_node_amd64(thread_node_amd64_t *node);\nint thread_add_child_amd64(thread_node_amd64_t *parent, thread_node_amd64_t *child);\n\nthread_node_amd64_t *thread_build_tree_amd64(message_thread_amd64_t *mailbox);\nchar *thread_format_response_amd64(thread_node_amd64_t *root);\n\n#endif\n