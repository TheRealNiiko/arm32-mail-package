#include "storage_termux32.h"
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <time.h>

#define MAX_MSG_SIZE (16 * 1024 * 1024)     /* ARM32: 16MB ceiling */

mailbox_storage_t32_t *storage_open_mailbox_t32(const char *maildir_path) {
    if (!maildir_path) return NULL;

    mailbox_storage_t32_t *mailbox = malloc(sizeof(mailbox_storage_t32_t));
    if (!mailbox) return NULL;

    strncpy(mailbox->maildir_path, maildir_path, 255);
    mailbox->maildir_path[255] = '\0';

    /* ARM32: start with 64 message slots — Termux mailboxes tend to be smaller */
    mailbox->messages = malloc(sizeof(message_metadata_t32_t) * 64);
    if (!mailbox->messages) {
        free(mailbox);
        return NULL;
    }

    mailbox->message_count    = 0;
    mailbox->message_capacity = 64;
    mailbox->next_uid         = 1;
    mailbox->uidvalidity      = (uint32_t)time(NULL);
    mailbox->modseq           = 1;
    pthread_rwlock_init(&mailbox->lock, NULL);

    return mailbox;
}

void storage_close_mailbox_t32(mailbox_storage_t32_t *mailbox) {
    if (!mailbox) return;

    pthread_rwlock_destroy(&mailbox->lock);
    free(mailbox->messages);
    free(mailbox);
}

int storage_add_message_t32(mailbox_storage_t32_t *mailbox, const char *data, uint32_t size) {
    if (!mailbox || !data || size == 0) return -1;
    if (size > MAX_MSG_SIZE) return -1;  /* ARM32: enforce 16MB ceiling */

    pthread_rwlock_wrlock(&mailbox->lock);

    if (mailbox->message_count >= mailbox->message_capacity) {
        mailbox->message_capacity *= 2;
        message_metadata_t32_t *grown = realloc(mailbox->messages,
            sizeof(message_metadata_t32_t) * mailbox->message_capacity);
        if (!grown) {
            pthread_rwlock_unlock(&mailbox->lock);
            return -1;
        }
        mailbox->messages = grown;
    }

    message_metadata_t32_t *msg = &mailbox->messages[mailbox->message_count];
    msg->uid      = mailbox->next_uid++;
    msg->size     = size;
    msg->date     = time(NULL);
    msg->seen     = 0;
    msg->flagged  = 0;
    msg->answered = 0;
    msg->deleted  = 0;
    msg->modseq   = mailbox->modseq++;

    snprintf(msg->filename, 512, "%s/cur/%u.eml", mailbox->maildir_path, msg->uid);

    mailbox->message_count++;
    pthread_rwlock_unlock(&mailbox->lock);

    return (int)msg->uid;
}

int storage_delete_message_t32(mailbox_storage_t32_t *mailbox, uint32_t uid) {
    if (!mailbox) return -1;

    pthread_rwlock_wrlock(&mailbox->lock);

    for (int i = 0; i < mailbox->message_count; i++) {
        if (mailbox->messages[i].uid == uid) {
            mailbox->messages[i].deleted = 1;
            mailbox->messages[i].modseq  = mailbox->modseq++;
            pthread_rwlock_unlock(&mailbox->lock);
            return 0;
        }
    }

    pthread_rwlock_unlock(&mailbox->lock);
    return -1;
}

int storage_mark_seen_t32(mailbox_storage_t32_t *mailbox, uint32_t uid) {
    if (!mailbox) return -1;

    pthread_rwlock_wrlock(&mailbox->lock);

    for (int i = 0; i < mailbox->message_count; i++) {
        if (mailbox->messages[i].uid == uid) {
            mailbox->messages[i].seen   = 1;
            mailbox->messages[i].modseq = mailbox->modseq++;
            pthread_rwlock_unlock(&mailbox->lock);
            return 0;
        }
    }

    pthread_rwlock_unlock(&mailbox->lock);
    return -1;
}

message_metadata_t32_t *storage_get_message_t32(mailbox_storage_t32_t *mailbox, uint32_t uid) {
    if (!mailbox) return NULL;

    pthread_rwlock_rdlock(&mailbox->lock);

    for (int i = 0; i < mailbox->message_count; i++) {
        if (mailbox->messages[i].uid == uid && !mailbox->messages[i].deleted) {
            message_metadata_t32_t *copy = malloc(sizeof(message_metadata_t32_t));
            if (copy)
                memcpy(copy, &mailbox->messages[i], sizeof(message_metadata_t32_t));
            pthread_rwlock_unlock(&mailbox->lock);
            return copy;
        }
    }

    pthread_rwlock_unlock(&mailbox->lock);
    return NULL;
}

char *storage_fetch_message_t32(mailbox_storage_t32_t *mailbox, uint32_t uid) {
    if (!mailbox) return NULL;

    message_metadata_t32_t *msg = storage_get_message_t32(mailbox, uid);
    if (!msg) return NULL;

    FILE *fp = fopen(msg->filename, "r");
    free(msg);
    if (!fp) return NULL;

    /* ARM32: cap read at 16MB */
    char *content = malloc(MAX_MSG_SIZE);
    if (!content) {
        fclose(fp);
        return NULL;
    }

    size_t n = fread(content, 1, MAX_MSG_SIZE - 1, fp);
    content[n] = '\0';
    fclose(fp);

    return content;
}

int storage_sync_mailbox_t32(mailbox_storage_t32_t *mailbox) {
    if (!mailbox) return -1;

    pthread_rwlock_rdlock(&mailbox->lock);
    /* Sync logic goes here */
    pthread_rwlock_unlock(&mailbox->lock);

    return 0;
}
