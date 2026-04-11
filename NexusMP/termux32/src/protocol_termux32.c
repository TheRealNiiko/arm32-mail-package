#include "protocol_termux32.h"
#include <ctype.h>
#include <stdio.h>
#include <sys/socket.h>

imap_command_t32_t *imap_parse_command_t32(const char *line) {
    if (!line) return NULL;

    imap_command_t32_t *cmd = malloc(sizeof(imap_command_t32_t));
    if (!cmd) return NULL;

    char *copy = strdup(line);
    if (!copy) {
        free(cmd);
        return NULL;
    }

    /* Strip CRLF */
    char *p = strchr(copy, '\r');
    if (p) *p = '\0';
    p = strchr(copy, '\n');
    if (p) *p = '\0';

    /* Parse: <tag> <command> [args] */
    char *space = strchr(copy, ' ');
    if (!space) {
        free(copy);
        free(cmd);
        return NULL;
    }

    *space    = '\0';
    cmd->tag  = strdup(copy);

    char *rest = space + 1;
    space = strchr(rest, ' ');

    if (space) {
        *space       = '\0';
        cmd->command = strdup(rest);
        cmd->args    = strdup(space + 1);
    } else {
        cmd->command = strdup(rest);
        cmd->args    = strdup("");
    }

    free(copy);
    return cmd;
}

void imap_free_command_t32(imap_command_t32_t *cmd) {
    if (!cmd) return;

    free(cmd->tag);
    free(cmd->command);
    free(cmd->args);
    free(cmd);
}

int imap_handle_capability_t32(int sock_fd, const char *tag) {
    if (sock_fd < 0 || !tag) return -1;

    /* ARM32: single stack buffer, smaller than amd64's 512 */
    char response[384];
    snprintf(response, sizeof(response),
        "* CAPABILITY IMAP4rev1 IDLE STARTTLS COMPRESS=DEFLATE AUTH=PLAIN AUTH=CRAM-MD5\r\n"
        "%s OK CAPABILITY completed\r\n", tag);

    send(sock_fd, response, strlen(response), 0);
    return 0;
}

int imap_handle_login_t32(int sock_fd, const char *tag, const char *username, const char *password) {
    if (sock_fd < 0 || !tag) return -1;
    (void)username; (void)password;

    char response[192];
    snprintf(response, sizeof(response),
        "%s OK [CAPABILITY IMAP4rev1 IDLE STARTTLS COMPRESS=DEFLATE] LOGIN completed\r\n", tag);

    send(sock_fd, response, strlen(response), 0);
    return 0;
}

int imap_handle_select_t32(int sock_fd, const char *tag, const char *mailbox_name) {
    if (sock_fd < 0 || !tag) return -1;
    (void)mailbox_name;

    char response[384];
    snprintf(response, sizeof(response),
        "* 0 EXISTS\r\n"
        "* 0 RECENT\r\n"
        "* OK [PERMANENTFLAGS (\\Seen \\Flagged \\Answered \\Deleted)]\r\n"
        "%s OK [READ-WRITE] SELECT completed\r\n", tag);

    send(sock_fd, response, strlen(response), 0);
    return 0;
}

int imap_handle_fetch_t32(int sock_fd, const char *tag,
                           const char *sequence_set, const char *fetch_items) {
    if (sock_fd < 0 || !tag) return -1;
    (void)sequence_set; (void)fetch_items;

    char response[128];
    snprintf(response, sizeof(response), "%s OK FETCH completed\r\n", tag);

    send(sock_fd, response, strlen(response), 0);
    return 0;
}

int imap_handle_search_t32(int sock_fd, const char *tag, const char *search_criteria) {
    if (sock_fd < 0 || !tag) return -1;
    (void)search_criteria;

    char response[128];
    snprintf(response, sizeof(response), "* SEARCH\r\n%s OK SEARCH completed\r\n", tag);

    send(sock_fd, response, strlen(response), 0);
    return 0;
}

int imap_handle_idle_t32(int sock_fd, const char *tag) {
    if (sock_fd < 0 || !tag) return -1;
    (void)tag;

    send(sock_fd, "+ idling\r\n", 10, 0);
    return 0;
}

int imap_handle_logout_t32(int sock_fd, const char *tag) {
    if (sock_fd < 0 || !tag) return -1;

    char response[192];
    snprintf(response, sizeof(response),
        "* BYE NexusMP Termux32 goodbye\r\n%s OK LOGOUT completed\r\n", tag);

    send(sock_fd, response, strlen(response), 0);
    return 0;
}

char *imap_format_response_t32(const char *tag, const char *status, const char *message) {
    if (!tag || !status) return NULL;

    /* ARM32: 256 bytes (was 512) */
    char *response = malloc(256);
    if (response)
        snprintf(response, 256, "%s %s %s\r\n", tag, status, message ? message : "");

    return response;
}
