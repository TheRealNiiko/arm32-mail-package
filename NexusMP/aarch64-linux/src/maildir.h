#ifndef MAILDIR_H
#define MAILDIR_H

#include "nexus_mp.h"

int maildir_init(const char *username, const char *maildir_path);
int maildir_list_messages(const char *maildir_path, message_t *messages, int max_messages);
int maildir_get_message(const char *maildir_path, uint32_t uid, message_t *msg);
int maildir_add_message(const char *maildir_path, const char *message_data, uint32_t size);
int maildir_delete_message(const char *maildir_path, uint32_t uid);
int maildir_set_flags(const char *maildir_path, uint32_t uid, uint32_t flags);
int maildir_get_flags(const char *maildir_path, uint32_t uid);

int read_message_headers(const char *filename, message_t *msg);
char *extract_header(const char *message_data, const char *header_name);

#endif
