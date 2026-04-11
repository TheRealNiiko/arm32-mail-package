#include "threading_amd64.h"
#include <time.h>
#include <stdio.h>

thread_node_amd64_t *thread_create_node_amd64(uint32_t uid) {
    thread_node_amd64_t *node = malloc(sizeof(thread_node_amd64_t));
    if (!node) return NULL;

    node->uid = uid;
    node->parent = NULL;
    node->children = malloc(sizeof(thread_node_amd64_t *) * 16);
    if (!node->children) {
        free(node);
        return NULL;
    }

    node->child_count = 0;
    node->child_capacity = 16;

    return node;
}

void thread_free_node_amd64(thread_node_amd64_t *node) {
    if (!node) return;

    for (int i = 0; i < node->child_count; i++) {
        thread_free_node_amd64(node->children[i]);
    }

    if (node->children) {
        free(node->children);
    }
    free(node);
}

int thread_add_child_amd64(thread_node_amd64_t *parent, thread_node_amd64_t *child) {
    if (!parent || !child) return -1;

    if (parent->child_count >= parent->child_capacity) {
        parent->child_capacity *= 2;
        thread_node_amd64_t **new_children = realloc(parent->children, 
                                                       sizeof(thread_node_amd64_t *) * parent->child_capacity);
        if (!new_children) return -1;
        parent->children = new_children;
    }

    parent->children[parent->child_count++] = child;
    child->parent = parent;

    return 0;
}

thread_node_amd64_t *thread_build_tree_amd64(message_thread_amd64_t *mailbox) {
    if (!mailbox) return NULL;

    thread_node_amd64_t *root = thread_create_node_amd64(0);
    if (!root) return NULL;

    pthread_rwlock_rdlock(&mailbox->lock);

    for (int i = 0; i < mailbox->message_count; i++) {
        thread_node_amd64_t *node = thread_create_node_amd64(mailbox->messages[i].uid);
        if (node) {
            thread_add_child_amd64(root, node);
        }
    }

    pthread_rwlock_unlock(&mailbox->lock);

    return root;
}

char *thread_format_response_amd64(thread_node_amd64_t *root) {
    if (!root) return strdup("* THREAD\r\n");

    char response[4096];
    strcpy(response, "* THREAD");

    /* Simple thread format output */
    for (int i = 0; i < root->child_count; i++) {
        char uid_str[32];
        snprintf(uid_str, sizeof(uid_str), " (%u)", root->children[i]->uid);
        strcat(response, uid_str);
    }

    strcat(response, "\r\n");

    return strdup(response);
}
