#ifndef ACL_TERMUX32_H
#define ACL_TERMUX32_H

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Same RFC 4314 right bits as amd64 */
#define ACL_LOOKUP     0x01
#define ACL_READ       0x02
#define ACL_WRITE      0x04
#define ACL_DELETE     0x08
#define ACL_ADMIN      0x10
#define ACL_ADMINISTER 0x20

typedef struct {
    char     identifier[32];    /* ARM32: tightened from 64 */
    uint32_t allow_rights;
    uint32_t deny_rights;
} acl_entry_t32_t;

typedef struct {
    acl_entry_t32_t *entries;
    int              entry_count;
    int              entry_capacity;
    pthread_rwlock_t lock;
} mailbox_acl_t32_t;

mailbox_acl_t32_t *acl_create_t32(void);
void acl_free_t32(mailbox_acl_t32_t *acl);

int  acl_grant_t32(mailbox_acl_t32_t *acl, const char *identifier, uint32_t rights);
int  acl_deny_t32(mailbox_acl_t32_t *acl, const char *identifier, uint32_t rights);
int  acl_revoke_t32(mailbox_acl_t32_t *acl, const char *identifier, uint32_t rights);

int   acl_check_permission_t32(mailbox_acl_t32_t *acl, const char *identifier, uint32_t required);
char *acl_list_t32(mailbox_acl_t32_t *acl);

#endif
