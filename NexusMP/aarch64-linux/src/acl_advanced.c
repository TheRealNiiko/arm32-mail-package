#include "acl.h"

static pthread_rwlock_t acl_global_lock = PTHREAD_RWLOCK_INITIALIZER;

acl_t *acl_create(void) {
    acl_t *acl = malloc(sizeof(acl_t));
    if (!acl) return NULL;
    
    pthread_rwlock_init(&acl->lock, NULL);
    acl->entries = malloc(100 * sizeof(acl_entry_t));
    if (!acl->entries) {
        free(acl);
        return NULL;
    }
    
    acl->entry_count = 0;
    acl->entry_capacity = 100;
    
    return acl;
}

void acl_free(acl_t *acl) {
    if (acl) {
        if (acl->entries) free(acl->entries);
        pthread_rwlock_destroy(&acl->lock);
        free(acl);
    }
}

int acl_grant(acl_t *acl, const char *identifier, uint32_t rights) {
    if (!acl || !identifier) return -1;
    
    pthread_rwlock_wrlock(&acl->lock);
    
    for (unsigned int i = 0; i < acl->entry_count; i++) {
        if (strcmp(acl->entries[i].identifier, identifier) == 0) {
            acl->entries[i].rights |= rights;
            acl->entries[i].deny = 0;
            pthread_rwlock_unlock(&acl->lock);
            return 0;
        }
    }
    
    if (acl->entry_count >= acl->entry_capacity) {
        acl->entry_capacity *= 2;
        acl_entry_t *new_entries = realloc(acl->entries, acl->entry_capacity * sizeof(acl_entry_t));
        if (!new_entries) {
            pthread_rwlock_unlock(&acl->lock);
            return -1;
        }
        acl->entries = new_entries;
    }
    
    strncpy(acl->entries[acl->entry_count].identifier, identifier, 63);
    acl->entries[acl->entry_count].identifier[63] = '\0';
    acl->entries[acl->entry_count].rights = rights;
    acl->entries[acl->entry_count].deny = 0;
    acl->entry_count++;
    
    pthread_rwlock_unlock(&acl->lock);
    return 0;
}

int acl_deny(acl_t *acl, const char *identifier, uint32_t rights) {
    if (!acl || !identifier) return -1;
    
    pthread_rwlock_wrlock(&acl->lock);
    
    for (unsigned int i = 0; i < acl->entry_count; i++) {
        if (strcmp(acl->entries[i].identifier, identifier) == 0) {
            acl->entries[i].rights &= ~rights;
            acl->entries[i].deny |= rights;
            pthread_rwlock_unlock(&acl->lock);
            return 0;
        }
    }
    
    if (acl->entry_count >= acl->entry_capacity) {
        acl->entry_capacity *= 2;
        acl_entry_t *new_entries = realloc(acl->entries, acl->entry_capacity * sizeof(acl_entry_t));
        if (!new_entries) {
            pthread_rwlock_unlock(&acl->lock);
            return -1;
        }
        acl->entries = new_entries;
    }
    
    strncpy(acl->entries[acl->entry_count].identifier, identifier, 63);
    acl->entries[acl->entry_count].identifier[63] = '\0';
    acl->entries[acl->entry_count].rights = 0;
    acl->entries[acl->entry_count].deny = rights;
    acl->entry_count++;
    
    pthread_rwlock_unlock(&acl->lock);
    return 0;
}

int acl_revoke(acl_t *acl, const char *identifier, uint32_t rights) {
    if (!acl || !identifier) return -1;
    
    pthread_rwlock_wrlock(&acl->lock);
    
    for (unsigned int i = 0; i < acl->entry_count; i++) {
        if (strcmp(acl->entries[i].identifier, identifier) == 0) {
            acl->entries[i].rights &= ~rights;
            acl->entries[i].deny &= ~rights;
            pthread_rwlock_unlock(&acl->lock);
            return 0;
        }
    }
    
    pthread_rwlock_unlock(&acl->lock);
    return -1;
}

uint32_t acl_lookup(acl_t *acl, const char *identifier) {
    if (!acl || !identifier) return 0;
    
    pthread_rwlock_rdlock(&acl->lock);
    
    for (unsigned int i = 0; i < acl->entry_count; i++) {
        if (strcmp(acl->entries[i].identifier, identifier) == 0) {
            uint32_t rights = acl->entries[i].rights;
            pthread_rwlock_unlock(&acl->lock);
            return rights;
        }
    }
    
    pthread_rwlock_unlock(&acl->lock);
    return 0;
}

int acl_check_permission(acl_t *acl, const char *identifier, uint32_t required) {
    uint32_t granted = acl_lookup(acl, identifier);
    return (granted & required) == required ? 1 : 0;
}

int acl_is_denied(acl_t *acl, const char *identifier, uint32_t right) {
    if (!acl || !identifier) return 0;
    
    pthread_rwlock_rdlock(&acl->lock);
    
    for (unsigned int i = 0; i < acl->entry_count; i++) {
        if (strcmp(acl->entries[i].identifier, identifier) == 0) {
            int denied = (acl->entries[i].deny & right) ? 1 : 0;
            pthread_rwlock_unlock(&acl->lock);
            return denied;
        }
    }
    
    pthread_rwlock_unlock(&acl->lock);
    return 0;
}

char *acl_list_rights(acl_t *acl) {
    char *response;
    char rights_str[128];
    
    response = malloc(4096);
    if (!response) return NULL;
    
    strcpy(response, "* ACL \"INBOX\"");
    
    pthread_rwlock_rdlock(&acl->lock);
    
    for (unsigned int i = 0; i < acl->entry_count; i++) {
        rights_str[0] = '\0';
        
        if (acl->entries[i].rights & ACL_LOOKUP) strcat(rights_str, "l");
        if (acl->entries[i].rights & ACL_READ) strcat(rights_str, "r");
        if (acl->entries[i].rights & ACL_WRITE) strcat(rights_str, "w");
        if (acl->entries[i].rights & ACL_DELETE) strcat(rights_str, "d");
        if (acl->entries[i].rights & ACL_ADMIN) strcat(rights_str, "a");
        
        if (rights_str[0]) {
            strcat(response, " ");
            strcat(response, acl->entries[i].identifier);
            strcat(response, " ");
            strcat(response, rights_str);
        }
    }
    
    pthread_rwlock_unlock(&acl->lock);
    
    strcat(response, "\r\n");
    
    return response;
}
