#ifndef ACL_H
#define ACL_H

#include "pthread.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ACL_LOOKUP 0x01
#define ACL_READ 0x02
#define ACL_WRITE 0x04
#define ACL_DELETE 0x08
#define ACL_ADMIN 0x10

typedef struct {
    char identifier[64];
    uint32_t rights;
    uint32_t deny;
} acl_entry_t;

typedef struct {
    acl_entry_t *entries;
    unsigned int entry_count;
    unsigned int entry_capacity;
    pthread_rwlock_t lock;
} acl_t;

acl_t *acl_create(void);
void acl_free(acl_t *acl);

int acl_grant(acl_t *acl, const char *identifier, uint32_t rights);
int acl_deny(acl_t *acl, const char *identifier, uint32_t rights);
int acl_revoke(acl_t *acl, const char *identifier, uint32_t rights);

uint32_t acl_lookup(acl_t *acl, const char *identifier);
int acl_check_permission(acl_t *acl, const char *identifier, uint32_t required);
int acl_is_denied(acl_t *acl, const char *identifier, uint32_t right);

char *acl_list_rights(acl_t *acl);

#endif
