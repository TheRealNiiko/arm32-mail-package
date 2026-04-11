#ifndef PROTOCOL_AMD64_H
#define PROTOCOL_AMD64_H

#include "pthread.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define IMAP_CAPABILITY_RESPONSE "* CAPABILITY IMAP4rev1 IDLE COMPRESS=DEFLATE STARTTLS AUTH=PLAIN AUTH=CRAM-MD5\r\n"
#define IMAP_OK_RESPONSE "OK\r\n"
#define IMAP_BAD_RESPONSE "BAD\r\n"
#define IMAP_NO_RESPONSE "NO\r\n"

typedef struct {
    char *tag;
    char *command;
    char *args;
} imap_command_amd64_t;

typedef enum {
    STATE_NOT_AUTHENTICATED,
    STATE_AUTHENTICATED,
    STATE_SELECTED,
    STATE_LOGOUT
} imap_state_amd64_t;

imap_command_amd64_t *imap_parse_command_amd64(const char *line);
void imap_free_command_amd64(imap_command_amd64_t *cmd);

int imap_handle_capability_amd64(int sock_fd, const char *tag);
int imap_handle_login_amd64(int sock_fd, const char *tag, const char *username, const char *password);
int imap_handle_select_amd64(int sock_fd, const char *tag, const char *mailbox_name);
int imap_handle_fetch_amd64(int sock_fd, const char *tag, const char *sequence_set, const char *fetch_items);
int imap_handle_search_amd64(int sock_fd, const char *tag, const char *search_criteria);
int imap_handle_idle_amd64(int sock_fd, const char *tag);
int imap_handle_logout_amd64(int sock_fd, const char *tag);

char *imap_format_response_amd64(const char *tag, const char *status, const char *message);

#endif
