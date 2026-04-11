#include "acl_amd64.h"

mailbox_acl_amd64_t *acl_create_amd64(void) {
    mailbox_acl_amd64_t *acl = malloc(sizeof(mailbox_acl_amd64_t));
    if (!acl) return NULL;

    acl->entries = malloc(sizeof(acl_entry_amd64_t) * 16);
    if (!acl->entries) {
        free(acl);
        return NULL;
    }

    acl->entry_count = 0;
    acl->entry_capacity = 16;
    pthread_rwlock_init(&acl->lock, NULL);

    return acl;
}

void acl_free_amd64(mailbox_acl_amd64_t *acl) {
    if (!acl) return;
    
    pthread_rwlock_destroy(&acl->lock);
    if (acl->entries) {
        free(acl->entries);
    }
    free(acl);
}

int acl_grant_amd64(mailbox_acl_amd64_t *acl, const char *identifier, uint32_t rights) {
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
        acl_entry_amd64_t *new_entries = realloc(acl->entries, sizeof(acl_entry_amd64_t) * acl->entry_capacity);
        if (!new_entries) {
            pthread_rwlock_unlock(&acl->lock);
            return -1;
        }
        acl->entries = new_entries;
    }

    acl_entry_amd64_t *entry = &acl->entries[acl->entry_count];
    strncpy(entry->identifier, identifier, 63);
    entry->identifier[63] = '\0';
    entry->allow_rights = rights;
    entry->deny_rights = 0;

    acl->entry_count++;
    pthread_rwlock_unlock(&acl->lock);

    return 0;
}

int acl_deny_amd64(mailbox_acl_amd64_t *acl, const char *identifier, uint32_t rights) {
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

int acl_revoke_amd64(mailbox_acl_amd64_t *acl, const char *identifier, uint32_t rights) {
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

int acl_check_permission_amd64(mailbox_acl_amd64_t *acl, const char *identifier, uint32_t required) {
    if (!acl || !identifier) return 0;

    pthread_rwlock_rdlock(&acl->lock);

    for (int i = 0; i < acl->entry_count; i++) {
        if (strcmp(acl->entries[i].identifier, identifier) == 0) {
            int has_perm = ((acl->entries[i].allow_rights & required) == required) && 
                          ((acl->entries[i].deny_rights & required) == 0);
            pthread_rwlock_unlock(&acl->lock);
            return has_perm;
        }
    }

    pthread_rwlock_unlock(&acl->lock);
    return 0;
}

char *acl_list_amd64(mailbox_acl_amd64_t *acl) {
    if (!acl) return strdup("* ACL INBOX\r\n");

    char *response = malloc(4096);
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
