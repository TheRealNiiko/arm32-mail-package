#include "search.h"
#include <regex.h>

static pthread_rwlock_t search_lock = PTHREAD_RWLOCK_INITIALIZER;

search_result_t *search_create_result(void) {
    search_result_t *result = malloc(sizeof(search_result_t));
    if (!result) return NULL;
    
    result->uid_capacity = 256;
    result->uid_count = 0;
    result->uids = malloc(result->uid_capacity * sizeof(uint32_t));
    
    if (!result->uids) {
        free(result);
        return NULL;
    }
    
    return result;
}

void search_free_result(search_result_t *result) {
    if (result) {
        if (result->uids) free(result->uids);
        free(result);
    }
}

void search_add_uid(search_result_t *result, uint32_t uid) {
    if (!result) return;
    
    if (result->uid_count >= result->uid_capacity) {
        result->uid_capacity *= 2;
        uint32_t *new_uids = realloc(result->uids, result->uid_capacity * sizeof(uint32_t));
        if (!new_uids) return;
        result->uids = new_uids;
    }
    
    result->uids[result->uid_count++] = uid;
}

int search_all_messages(mailbox_t *mailbox, search_result_t *result) {
    uint32_t i;
    
    if (!mailbox || !result) return -1;
    
    pthread_rwlock_rdlock(&mailbox->lock);
    
    for (i = 0; i < mailbox->message_count; i++) {
        search_add_uid(result, mailbox->messages[i].uid);
    }
    
    pthread_rwlock_unlock(&mailbox->lock);
    return 0;
}

int search_by_flags(mailbox_t *mailbox, uint64_t flags, int match_all, search_result_t *result) {
    uint32_t i;
    
    if (!mailbox || !result) return -1;
    
    pthread_rwlock_rdlock(&mailbox->lock);
    
    for (i = 0; i < mailbox->message_count; i++) {
        int match = 0;
        
        if (match_all) {
            if ((mailbox->messages[i].flags & flags) == flags) {
                match = 1;
            }
        } else {
            if (mailbox->messages[i].flags & flags) {
                match = 1;
            }
        }
        
        if (match) {
            search_add_uid(result, mailbox->messages[i].uid);
        }
    }
    
    pthread_rwlock_unlock(&mailbox->lock);
    return 0;
}

int search_by_date_range(mailbox_t *mailbox, time_t start, time_t end, search_result_t *result) {
    uint32_t i;
    
    if (!mailbox || !result) return -1;
    
    pthread_rwlock_rdlock(&mailbox->lock);
    
    for (i = 0; i < mailbox->message_count; i++) {
        if (mailbox->messages[i].date >= start && mailbox->messages[i].date <= end) {
            search_add_uid(result, mailbox->messages[i].uid);
        }
    }
    
    pthread_rwlock_unlock(&mailbox->lock);
    return 0;
}

int search_by_text(mailbox_t *mailbox, const char *text, const char *field, search_result_t *result) {
    uint32_t i;
    const char *search_field = NULL;
    
    if (!mailbox || !result || !text || !field) return -1;
    
    pthread_rwlock_rdlock(&mailbox->lock);
    
    for (i = 0; i < mailbox->message_count; i++) {
        search_field = NULL;
        
        if (strcmp(field, "SUBJECT") == 0) {
            search_field = mailbox->messages[i].subject;
        } else if (strcmp(field, "FROM") == 0) {
            search_field = mailbox->messages[i].from;
        } else if (strcmp(field, "TO") == 0) {
            search_field = mailbox->messages[i].to;
        } else if (strcmp(field, "CC") == 0) {
            search_field = mailbox->messages[i].cc;
        } else if (strcmp(field, "BCC") == 0) {
            search_field = mailbox->messages[i].bcc;
        }
        
        if (search_field && strcasestr(search_field, text)) {
            search_add_uid(result, mailbox->messages[i].uid);
        }
    }
    
    pthread_rwlock_unlock(&mailbox->lock);
    return 0;
}

int search_by_regex(mailbox_t *mailbox, const char *pattern, search_result_t *result) {
    regex_t regex;
    uint32_t i;
    int reti;
    
    if (!mailbox || !result || !pattern) return -1;
    
    reti = regcomp(&regex, pattern, REG_EXTENDED | REG_ICASE);
    if (reti) {
        return -1;
    }
    
    pthread_rwlock_rdlock(&mailbox->lock);
    
    for (i = 0; i < mailbox->message_count; i++) {
        if (regexec(&regex, mailbox->messages[i].subject, 0, NULL, 0) == 0 ||
            regexec(&regex, mailbox->messages[i].from, 0, NULL, 0) == 0 ||
            regexec(&regex, mailbox->messages[i].to, 0, NULL, 0) == 0) {
            search_add_uid(result, mailbox->messages[i].uid);
        }
    }
    
    pthread_rwlock_unlock(&mailbox->lock);
    regfree(&regex);
    
    return 0;
}

char *format_search_result(search_result_t *result) {
    char *response;
    size_t response_size;
    size_t response_used;
    uint32_t i;
    
    if (!result || result->uid_count == 0) {
        response = malloc(32);
        strcpy(response, "* SEARCH\r\n");
        return response;
    }
    
    response_size = result->uid_count * 16 + 64;
    response = malloc(response_size);
    if (!response) return NULL;
    
    strcpy(response, "* SEARCH");
    response_used = strlen(response);
    
    for (i = 0; i < result->uid_count; i++) {
        int written = snprintf(response + response_used, 
                              response_size - response_used, 
                              " %u", result->uids[i]);
        response_used += written;
    }
    
    strcat(response, "\r\n");
    
    return response;
}
