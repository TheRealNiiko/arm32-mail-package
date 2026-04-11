#include "storage_amd64.h"
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>

mailbox_storage_amd64_t *storage_open_mailbox_amd64(const char *maildir_path) {
    if (!maildir_path) return NULL;

    mailbox_storage_amd64_t *mailbox = malloc(sizeof(mailbox_storage_amd64_t));
    if (!mailbox) return NULL;

    strncpy(mailbox->maildir_path, maildir_path, 511);
    mailbox->maildir_path[511] = '\0';

    mailbox->messages = malloc(sizeof(message_metadata_amd64_t) * 256);
    if (!mailbox->messages) {
        free(mailbox);
        return NULL;
    }

    mailbox->message_count = 0;
    mailbox->message_capacity = 256;
    mailbox->next_uid = 1;
    mailbox->uidvalidity = time(NULL);
    mailbox->modseq = 1;
    pthread_rwlock_init(&mailbox->lock, NULL);

    return mailbox;
}

void storage_close_mailbox_amd64(mailbox_storage_amd64_t *mailbox) {
    if (!mailbox) return;
    
    pthread_rwlock_destroy(&mailbox->lock);
    if (mailbox->messages) {
        free(mailbox->messages);
    }
    free(mailbox);
}

int storage_add_message_amd64(mailbox_storage_amd64_t *mailbox, const char *message_data, uint64_t size) {
    if (!mailbox || !message_data || size == 0) return -1;

    pthread_rwlock_wrlock(&mailbox->lock);

    if (mailbox->message_count >= mailbox->message_capacity) {
        mailbox->message_capacity *= 2;
        message_metadata_amd64_t *new_msgs = realloc(mailbox->messages, 
                                                      sizeof(message_metadata_amd64_t) * mailbox->message_capacity);
        if (!new_msgs) {
            pthread_rwlock_unlock(&mailbox->lock);
            return -1;
        }
        mailbox->messages = new_msgs;
    }

    message_metadata_amd64_t *msg = &mailbox->messages[mailbox->message_count];
    msg->uid = mailbox->next_uid++;
    msg->size = size;
    msg->date = time(NULL);
    msg->seen = 0;
    msg->flagged = 0;
    msg->answered = 0;
    msg->deleted = 0;
    msg->modseq = mailbox->modseq++;
    
    snprintf(msg->filename, 511, "%s/cur/%u.eml", mailbox->maildir_path, msg->uid);

    mailbox->message_count++;
    pthread_rwlock_unlock(&mailbox->lock);

    return msg->uid;
}

int storage_delete_message_amd64(mailbox_storage_amd64_t *mailbox, uint32_t uid) {
    if (!mailbox) return -1;

    pthread_rwlock_wrlock(&mailbox->lock);

    for (int i = 0; i < mailbox->message_count; i++) {
        if (mailbox->messages[i].uid == uid) {
            mailbox->messages[i].deleted = 1;
            mailbox->messages[i].modseq = mailbox->modseq++;
            pthread_rwlock_unlock(&mailbox->lock);
            return 0;
        }
    }

    pthread_rwlock_unlock(&mailbox->lock);
    return -1;
}

int storage_mark_seen_amd64(mailbox_storage_amd64_t *mailbox, uint32_t uid) {
    if (!mailbox) return -1;

    pthread_rwlock_wrlock(&mailbox->lock);

    for (int i = 0; i < mailbox->message_count; i++) {
        if (mailbox->messages[i].uid == uid) {
            mailbox->messages[i].seen = 1;
            mailbox->messages[i].modseq = mailbox->modseq++;
            pthread_rwlock_unlock(&mailbox->lock);
            return 0;
        }
    }

    pthread_rwlock_unlock(&mailbox->lock);
    return -1;
}

message_metadata_amd64_t *storage_get_message_amd64(mailbox_storage_amd64_t *mailbox, uint32_t uid) {
    if (!mailbox) return NULL;

    pthread_rwlock_rdlock(&mailbox->lock);

    for (int i = 0; i < mailbox->message_count; i++) {
        if (mailbox->messages[i].uid == uid && !mailbox->messages[i].deleted) {
            message_metadata_amd64_t *result = malloc(sizeof(message_metadata_amd64_t));
            if (result) {
                memcpy(result, &mailbox->messages[i], sizeof(message_metadata_amd64_t));
            }
            pthread_rwlock_unlock(&mailbox->lock);
            return result;
        }
    }

    pthread_rwlock_unlock(&mailbox->lock);
    return NULL;
}

char *storage_fetch_message_amd64(mailbox_storage_amd64_t *mailbox, uint32_t uid) {
    if (!mailbox) return NULL;

    message_metadata_amd64_t *msg = storage_get_message_amd64(mailbox, uid);
    if (!msg) return NULL;

    FILE *fp = fopen(msg->filename, "r");
    free(msg);
    if (!fp) return NULL;

    char *content = malloc(MAX_MESSAGE_SIZE);
    if (!content) {
        fclose(fp);
        return NULL;
    }

    size_t n = fread(content, 1, MAX_MESSAGE_SIZE - 1, fp);
    content[n] = '\0';
    fclose(fp);

    return content;
}

int storage_sync_mailbox_amd64(mailbox_storage_amd64_t *mailbox) {
    if (!mailbox) return -1;
    
    pthread_rwlock_rdlock(&mailbox->lock);
    /* Sync logic would go here */
    pthread_rwlock_unlock(&mailbox->lock);

    return 0;
}
