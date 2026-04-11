#include "threading_termux32.h"
#include <stdio.h>

thread_node_t32_t *thread_create_node_t32(uint32_t uid) {
    thread_node_t32_t *node = malloc(sizeof(thread_node_t32_t));
    if (!node) return NULL;

    node->uid           = uid;
    node->parent        = NULL;
    /* ARM32: start with 8 child slots (was 16) */
    node->children      = malloc(sizeof(thread_node_t32_t *) * 8);
    if (!node->children) {
        free(node);
        return NULL;
    }

    node->child_count    = 0;
    node->child_capacity = 8;

    return node;
}

void thread_free_node_t32(thread_node_t32_t *node) {
    if (!node) return;

    for (int i = 0; i < node->child_count; i++)
        thread_free_node_t32(node->children[i]);

    free(node->children);
    free(node);
}

int thread_add_child_t32(thread_node_t32_t *parent, thread_node_t32_t *child) {
    if (!parent || !child) return -1;

    if (parent->child_count >= parent->child_capacity) {
        parent->child_capacity *= 2;
        thread_node_t32_t **grown = realloc(parent->children,
            sizeof(thread_node_t32_t *) * parent->child_capacity);
        if (!grown) return -1;
        parent->children = grown;
    }

    parent->children[parent->child_count++] = child;
    child->parent = parent;

    return 0;
}

thread_node_t32_t *thread_build_tree_t32(message_thread_t32_t *mailbox) {
    if (!mailbox) return NULL;

    thread_node_t32_t *root = thread_create_node_t32(0);
    if (!root) return NULL;

    pthread_rwlock_rdlock(&mailbox->lock);

    for (int i = 0; i < mailbox->message_count; i++) {
        thread_node_t32_t *node = thread_create_node_t32(mailbox->messages[i].uid);
        if (node)
            thread_add_child_t32(root, node);
    }

    pthread_rwlock_unlock(&mailbox->lock);

    return root;
}

char *thread_format_response_t32(thread_node_t32_t *root) {
    if (!root) return strdup("* THREAD\r\n");

    /* ARM32: smaller response buffer (2048 vs 4096) */
    char response[2048];
    strcpy(response, "* THREAD");

    for (int i = 0; i < root->child_count; i++) {
        char uid_str[16];
        snprintf(uid_str, sizeof(uid_str), " (%u)", root->children[i]->uid);
        /* Guard against overflow */
        if (strlen(response) + strlen(uid_str) + 3 < sizeof(response))
            strcat(response, uid_str);
    }

    strcat(response, "\r\n");
    return strdup(response);
}
