#include "acl_termux32.h"
#include <stdio.h>

mailbox_acl_t32_t *acl_create_t32(void) {
    mailbox_acl_t32_t *acl = malloc(sizeof(mailbox_acl_t32_t));
    if (!acl) return NULL;

    /* ARM32: start smaller */
    acl->entries = malloc(sizeof(acl_entry_t32_t) * 8);
    if (!acl->entries) {
        free(acl);
        return NULL;
    }

    acl->entry_count    = 0;
    acl->entry_capacity = 8;
    pthread_rwlock_init(&acl->lock, NULL);

    return acl;
}

void acl_free_t32(mailbox_acl_t32_t *acl) {
    if (!acl) return;

    pthread_rwlock_destroy(&acl->lock);
    free(acl->entries);
    free(acl);
}

int acl_grant_t32(mailbox_acl_t32_t *acl, const char *identifier, uint32_t rights) {
    if (!acl || !identifier) return -1;

    pthread_rwlock_wrlock(&acl->lock);

    for (int i = 0; i < acl->entry_count; i++) {
        if (strcmp(acl->entries[i].identifier, identifier) == 0) {
            acl->entries[i].allow_rights |= rights;
            pthread_rwlock_unlock(&acl->lock);
            return 0;
        }
    }

    if (acl->entry_count >= acl->entry_capacity) {
        acl->entry_capacity *= 2;
        acl_entry_t32_t *grown = realloc(acl->entries,
                                         sizeof(acl_entry_t32_t) * acl->entry_capacity);
        if (!grown) {
            pthread_rwlock_unlock(&acl->lock);
            return -1;
        }
        acl->entries = grown;
    }

    acl_entry_t32_t *entry = &acl->entries[acl->entry_count];
    strncpy(entry->identifier, identifier, 31);
    entry->identifier[31] = '\0';
    entry->allow_rights   = rights;
    entry->deny_rights    = 0;

    acl->entry_count++;
    pthread_rwlock_unlock(&acl->lock);

    return 0;
}

int acl_deny_t32(mailbox_acl_t32_t *acl, const char *identifier, uint32_t rights) {
    if (!acl || !identifier) return -1;

    pthread_rwlock_wrlock(&acl->lock);

    for (int i = 0; i < acl->entry_count; i++) {
        if (strcmp(acl->entries[i].identifier, identifier) == 0) {
            acl->entries[i].deny_rights |= rights;
            pthread_rwlock_unlock(&acl->lock);
            return 0;
        }
    }

    pthread_rwlock_unlock(&acl->lock);
    return -1;
}

int acl_revoke_t32(mailbox_acl_t32_t *acl, const char *identifier, uint32_t rights) {
    if (!acl || !identifier) return -1;

    pthread_rwlock_wrlock(&acl->lock);

    for (int i = 0; i < acl->entry_count; i++) {
        if (strcmp(acl->entries[i].identifier, identifier) == 0) {
            acl->entries[i].allow_rights &= ~rights;
            pthread_rwlock_unlock(&acl->lock);
            return 0;
        }
    }

    pthread_rwlock_unlock(&acl->lock);
    return -1;
}

int acl_check_permission_t32(mailbox_acl_t32_t *acl, const char *identifier, uint32_t required) {
    if (!acl || !identifier) return 0;

    pthread_rwlock_rdlock(&acl->lock);

    for (int i = 0; i < acl->entry_count; i++) {
        if (strcmp(acl->entries[i].identifier, identifier) == 0) {
            int ok = ((acl->entries[i].allow_rights & required) == required) &&
                     ((acl->entries[i].deny_rights  & required) == 0);
            pthread_rwlock_unlock(&acl->lock);
            return ok;
        }
    }

    pthread_rwlock_unlock(&acl->lock);
    return 0;
}

char *acl_list_t32(mailbox_acl_t32_t *acl) {
    if (!acl) return strdup("* ACL INBOX\r\n");

    /* ARM32: smaller ACL response buffer */
    char *response = malloc(2048);
    if (!response) return NULL;

    strcpy(response, "* ACL INBOX");

    pthread_rwlock_rdlock(&acl->lock);
    for (int i = 0; i < acl->entry_count; i++) {
        strcat(response, " \"");
        strcat(response, acl->entries[i].identifier);
        strcat(response, "\" \"\"");
    }
    pthread_rwlock_unlock(&acl->lock);

    strcat(response, "\r\n");
    return response;
}
