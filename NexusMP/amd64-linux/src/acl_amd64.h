#ifndef ACL_AMD64_H
#define ACL_AMD64_H

#include "pthread.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ACL_LOOKUP_A 0x01
#define ACL_READ_R 0x02
#define ACL_WRITE_W 0x04
#define ACL_DELETE_D 0x08
#define ACL_ADMIN_A 0x10
#define ACL_ADMINISTER 0x20

typedef struct {
    char identifier[64];
    uint32_t allow_rights;
    uint32_t deny_rights;
} acl_entry_amd64_t;

typedef struct {
    acl_entry_amd64_t *entries;
    int entry_count;
    int entry_capacity;
    pthread_rwlock_t lock;
} mailbox_acl_amd64_t;

mailbox_acl_amd64_t *acl_create_amd64(void);
void acl_free_amd64(mailbox_acl_amd64_t *acl);

int acl_grant_amd64(mailbox_acl_amd64_t *acl, const char *identifier, uint32_t rights);
int acl_deny_amd64(mailbox_acl_amd64_t *acl, const char *identifier, uint32_t rights);
int acl_revoke_amd64(mailbox_acl_amd64_t *acl, const char *identifier, uint32_t rights);

int acl_check_permission_amd64(mailbox_acl_amd64_t *acl, const char *identifier, uint32_t required);
char *acl_list_amd64(mailbox_acl_amd64_t *acl);

#endif
