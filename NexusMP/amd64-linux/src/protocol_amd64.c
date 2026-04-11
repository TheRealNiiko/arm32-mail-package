#include "protocol_amd64.h"
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/socket.h>

imap_command_amd64_t *imap_parse_command_amd64(const char *line) {
    if (!line) return NULL;

    imap_command_amd64_t *cmd = malloc(sizeof(imap_command_amd64_t));
    if (!cmd) return NULL;

    char *copy = strdup(line);
    if (!copy) {
        free(cmd);
        return NULL;
    }

    /* Remove trailing CRLF */
    char *p = strchr(copy, '\r');
    if (p) *p = '\0';
    p = strchr(copy, '\n');
    if (p) *p = '\0';

    /* Parse tag */
    char *space = strchr(copy, ' ');
    if (!space) {
        free(copy);
        free(cmd);
        return NULL;
    }

    *space = '\0';
    cmd->tag = strdup(copy);
    
    /* Parse command */
    char *rest = space + 1;
    space = strchr(rest, ' ');
    
    if (space) {
        *space = '\0';
        cmd->command = strdup(rest);
        cmd->args = strdup(space + 1);
    } else {
        cmd->command = strdup(rest);
        cmd->args = strdup("");
    }

    free(copy);
    return cmd;
}

void imap_free_command_amd64(imap_command_amd64_t *cmd) {
    if (!cmd) return;
    
    if (cmd->tag) free(cmd->tag);
    if (cmd->command) free(cmd->command);
    if (cmd->args) free(cmd->args);
    free(cmd);
}

int imap_handle_capability_amd64(int sock_fd, const char *tag) {
    if (sock_fd < 0 || !tag) return -1;

    char response[512];
    snprintf(response, sizeof(response), "* CAPABILITY IMAP4rev1 IDLE STARTTLS COMPRESS=DEFLATE AUTH=PLAIN AUTH=CRAM-MD5\r\n%s OK CAPABILITY completed\r\n", tag);
    
    send(sock_fd, response, strlen(response), 0);
    return 0;
}

int imap_handle_login_amd64(int sock_fd, const char *tag, const char *username, const char *password) {
    if (sock_fd < 0 || !tag) return -1;

    char response[256];
    snprintf(response, sizeof(response), "%s OK [CAPABILITY IMAP4rev1 IDLE STARTTLS COMPRESS=DEFLATE] LOGIN completed\r\n", tag);
    
    send(sock_fd, response, strlen(response), 0);
    return 0;
}

int imap_handle_select_amd64(int sock_fd, const char *tag, const char *mailbox_name) {
    if (sock_fd < 0 || !tag) return -1;

    char response[512];
    snprintf(response, sizeof(response), "* 0 EXISTS\r\n* 0 RECENT\r\n* OK [PERMANENTFLAGS (\\Seen \\Flagged \\Answered \\Deleted)]\r\n%s OK [READ-WRITE] SELECT completed\r\n", tag);
    
    send(sock_fd, response, strlen(response), 0);
    return 0;
}

int imap_handle_fetch_amd64(int sock_fd, const char *tag, const char *sequence_set, const char *fetch_items) {
    if (sock_fd < 0 || !tag) return -1;

    char response[256];
    snprintf(response, sizeof(response), "%s OK FETCH completed\r\n", tag);
    
    send(sock_fd, response, strlen(response), 0);
    return 0;
}

int imap_handle_search_amd64(int sock_fd, const char *tag, const char *search_criteria) {
    if (sock_fd < 0 || !tag) return -1;

    char response[256];
    snprintf(response, sizeof(response), "* SEARCH\r\n%s OK SEARCH completed\r\n", tag);
    
    send(sock_fd, response, strlen(response), 0);
    return 0;
}

int imap_handle_idle_amd64(int sock_fd, const char *tag) {
    if (sock_fd < 0 || !tag) return -1;

    send(sock_fd, "+ idling\r\n", 10, 0);
    return 0;
}

int imap_handle_logout_amd64(int sock_fd, const char *tag) {
    if (sock_fd < 0 || !tag) return -1;

    char response[256];
    snprintf(response, sizeof(response), "* BYE NexusMP AMD64 goodbye\r\n%s OK LOGOUT completed\r\n", tag);
    
    send(sock_fd, response, strlen(response), 0);
    return 0;
}

char *imap_format_response_amd64(const char *tag, const char *status, const char *message) {
    if (!tag || !status) return NULL;

    char *response = malloc(512);
    if (response) {
        snprintf(response, 512, "%s %s %s\r\n", tag, status, message ? message : "");
    }

    return response;
}
