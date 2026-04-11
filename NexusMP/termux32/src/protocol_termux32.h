#ifndef PROTOCOL_TERMUX32_H
#define PROTOCOL_TERMUX32_H

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define IMAP_CAPABILITY_RESPONSE \
    "* CAPABILITY IMAP4rev1 IDLE COMPRESS=DEFLATE STARTTLS AUTH=PLAIN AUTH=CRAM-MD5\r\n"
#define IMAP_OK_RESPONSE  "OK\r\n"
#define IMAP_BAD_RESPONSE "BAD\r\n"
#define IMAP_NO_RESPONSE  "NO\r\n"

typedef struct {
    char *tag;
    char *command;
    char *args;
} imap_command_t32_t;

typedef enum {
    STATE_NOT_AUTHENTICATED,
    STATE_AUTHENTICATED,
    STATE_SELECTED,
    STATE_LOGOUT
} imap_state_t32_t;

imap_command_t32_t *imap_parse_command_t32(const char *line);
void imap_free_command_t32(imap_command_t32_t *cmd);

int imap_handle_capability_t32(int sock_fd, const char *tag);
int imap_handle_login_t32(int sock_fd, const char *tag, const char *username, const char *password);
int imap_handle_select_t32(int sock_fd, const char *tag, const char *mailbox_name);
int imap_handle_fetch_t32(int sock_fd, const char *tag, const char *sequence_set, const char *fetch_items);
int imap_handle_search_t32(int sock_fd, const char *tag, const char *search_criteria);
int imap_handle_idle_t32(int sock_fd, const char *tag);
int imap_handle_logout_t32(int sock_fd, const char *tag);

char *imap_format_response_t32(const char *tag, const char *status, const char *message);

#endif
