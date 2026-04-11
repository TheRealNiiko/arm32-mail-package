#include "threading.h"
#include <string.h>

static pthread_rwlock_t thread_lock = PTHREAD_RWLOCK_INITIALIZER;

thread_node_t *thread_create_node(uint32_t uid) {
    thread_node_t *node = malloc(sizeof(thread_node_t));
    if (!node) return NULL;
    
    node->uid = uid;
    node->parent = NULL;
    node->children = malloc(10 * sizeof(thread_node_t *));
    if (!node->children) {
        free(node);
        return NULL;
    }
    node->child_count = 0;
    node->child_capacity = 10;
    
    return node;
}

void thread_free_node(thread_node_t *node) {
    if (node) {
        if (node->children) {
            for (unsigned int i = 0; i < node->child_count; i++) {
                thread_free_node(node->children[i]);
            }
            free(node->children);
        }
        free(node);
    }
}

int thread_add_child(thread_node_t *parent, thread_node_t *child) {
    if (!parent || !child) return -1;
    
    if (parent->child_count >= parent->child_capacity) {
        parent->child_capacity *= 2;
        thread_node_t **new_children = realloc(parent->children, 
                                               parent->child_capacity * sizeof(thread_node_t *));
        if (!new_children) return -1;
        parent->children = new_children;
    }
    
    parent->children[parent->child_count++] = child;
    child->parent = parent;
    
    return 0;
}

thread_node_t *thread_parse_references(const char *references, mailbox_t *mailbox) {
    thread_node_t *root = NULL, *current = NULL, *node = NULL;
    char *refs_copy, *saveptr, *msg_id;
    uint32_t uid;
    
    if (!references || !mailbox) return NULL;
    
    refs_copy = strdup(references);
    if (!refs_copy) return NULL;
    
    msg_id = strtok_r(refs_copy, " ", &saveptr);
    
    while (msg_id) {
        uid = storage_find_uid_by_message_id(mailbox, msg_id);
        if (uid > 0) {
            node = thread_create_node(uid);
            if (!node) continue;
            
            if (!root) {
                root = node;
                current = node;
            } else {
                thread_add_child(current, node);
                current = node;
            }
        }
        msg_id = strtok_r(NULL, " ", &saveptr);
    }
    
    free(refs_copy);
    return root;
}

int thread_build_tree(mailbox_t *mailbox, thread_node_t **root) {
    thread_node_t *node, *parent_node;
    uint32_t i;
    
    if (!mailbox || !root) return -1;
    
    pthread_rwlock_wrlock(&thread_lock);
    
    *root = thread_create_node(0);
    if (!*root) {
        pthread_rwlock_unlock(&thread_lock);
        return -1;
    }
    
    for (i = 0; i < mailbox->message_count; i++) {
        node = thread_create_node(mailbox->messages[i].uid);
        if (!node) continue;
        
        if (mailbox->messages[i].reference_count > 0) {
            parent_node = thread_parse_references(mailbox->messages[i].references, mailbox);
            if (parent_node) {
                thread_add_child(parent_node, node);
            } else {
                thread_add_child(*root, node);
            }
        } else {
            thread_add_child(*root, node);
        }
    }
    
    pthread_rwlock_unlock(&thread_lock);
    return 0;
}

static int thread_format_recursive(thread_node_t *node, char *buffer, size_t remaining) {
    size_t written = 0;
    unsigned int i;
    
    if (!node || remaining < 10) return -1;
    
    written += snprintf(buffer + written, remaining - written, "(");
    
    if (node->uid > 0) {
        written += snprintf(buffer + written, remaining - written, "%u", node->uid);
    }
    
    if (node->child_count > 0) {
        if (node->uid > 0) {
            written += snprintf(buffer + written, remaining - written, " ");
        }
        for (i = 0; i < node->child_count; i++) {
            if (thread_format_recursive(node->children[i], buffer + written, remaining - written) < 0) {
                return -1;
            }
            written = strlen(buffer);
            if (i < node->child_count - 1) {
                written += snprintf(buffer + written, remaining - written, " ");
            }
        }
    }
    
    written += snprintf(buffer + written, remaining - written, ")");
    
    return 0;
}

char *thread_format_response(thread_node_t *root) {
    char *response;
    
    if (!root) return NULL;
    
    response = malloc(8192);
    if (!response) return NULL;
    
    strcpy(response, "* THREAD ");
    
    if (thread_format_recursive(root, response + strlen(response), 8192 - strlen(response)) < 0) {
        free(response);
        return NULL;
    }
    
    strcat(response, "\r\n");
    
    return response;
}

int thread_update_modseq(thread_node_t *node, uint64_t *modseq) {
    unsigned int i;
    
    if (!node || !modseq) return -1;
    
    if (node->uid > 0) {
        (*modseq)++;
    }
    
    for (i = 0; i < node->child_count; i++) {
        if (thread_update_modseq(node->children[i], modseq) < 0) {
            return -1;
        }
    }
    
    return 0;
}
