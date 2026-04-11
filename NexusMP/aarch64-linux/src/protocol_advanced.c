#include "protocol.h"

static pthread_rwlock_t proto_lock = PTHREAD_RWLOCK_INITIALIZER;

typedef struct {
    const char *command;
    int (*handler)(client_t *client, const char *args, mailbox_t *mailbox);
} command_handler_t;

static int handle_capability(client_t *client, const char *args, mailbox_t *mailbox) {
    const char *response = "* CAPABILITY IMAP4rev1 STARTTLS COMPRESS=DEFLATE IDLE NAMESPACE ID CHILDREN\r\n"
                          "OK CAPABILITY completed\r\n";
    send(client->sock, response, strlen(response), MSG_NOSIGNAL);
    return 0;
}

static int handle_login(client_t *client, const char *args, mailbox_t *mailbox) {
    char username[256], password[256];
    int parsed;
    
    if (!args) {
        send(client->sock, "BAD invalid LOGIN arguments\r\n", 30, MSG_NOSIGNAL);
        return -1;
    }
    
    parsed = sscanf(args, "%255s %255s", username, password);
    if (parsed != 2) {
        send(client->sock, "BAD invalid LOGIN format\r\n", 27, MSG_NOSIGNAL);
        return -1;
    }
    
    if (auth_verify_password(username, password) == 0) {
        client->authenticated = 1;
        strncpy(client->username, username, 63);
        send(client->sock, "OK LOGIN completed\r\n", 20, MSG_NOSIGNAL);
        return 0;
    }
    
    send(client->sock, "NO LOGIN failed\r\n", 17, MSG_NOSIGNAL);
    return -1;
}

static int handle_select(client_t *client, const char *args, mailbox_t *current_mailbox) {
    mailbox_t *mailbox;
    char mailbox_name[256];
    char response[512];
    
    if (!client->authenticated) {
        send(client->sock, "NO not authenticated\r\n", 22, MSG_NOSIGNAL);
        return -1;
    }
    
    if (sscanf(args, "%255s", mailbox_name) != 1) {
        send(client->sock, "BAD invalid SELECT arguments\r\n", 30, MSG_NOSIGNAL);
        return -1;
    }
    
    mailbox = storage_get_mailbox(client->username, mailbox_name);
    if (!mailbox) {
        send(client->sock, "NO mailbox does not exist\r\n", 27, MSG_NOSIGNAL);
        return -1;
    }
    
    snprintf(response, sizeof(response),
            "* %u EXISTS\r\n"
            "* %u RECENT\r\n"
            "* FLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft)\r\n"
            "OK SELECT completed\r\n",
            mailbox->message_count,
            mailbox->recent_count);
    
    send(client->sock, response, strlen(response), MSG_NOSIGNAL);
    return 0;
}

static int handle_fetch(client_t *client, const char *args, mailbox_t *mailbox) {
    uint32_t uid;
    message_t *msg;
    char response[2048];
    
    if (!client->authenticated || !mailbox) {
        send(client->sock, "NO not in appropriate state\r\n", 30, MSG_NOSIGNAL);
        return -1;
    }
    
    if (sscanf(args, "%u", &uid) != 1) {
        send(client->sock, "BAD invalid FETCH arguments\r\n", 30, MSG_NOSIGNAL);
        return -1;
    }
    
    msg = storage_get_message(mailbox, uid);
    if (!msg) {
        send(client->sock, "NO message not found\r\n", 23, MSG_NOSIGNAL);
        return -1;
    }
    
    snprintf(response, sizeof(response),
            "* %u FETCH (UID %u RFC822.SIZE %u FLAGS (\\Seen))\r\n"
            "OK FETCH completed\r\n",
            uid, uid, (unsigned int)msg->size);
    
    send(client->sock, response, strlen(response), MSG_NOSIGNAL);
    return 0;
}

static int handle_search(client_t *client, const char *args, mailbox_t *mailbox) {
    search_result_t *result;
    char *response;
    
    if (!client->authenticated || !mailbox) {
        send(client->sock, "NO not in appropriate state\r\n", 30, MSG_NOSIGNAL);
        return -1;
    }
    
    result = search_create_result();
    if (!result) return -1;
    
    search_all_messages(mailbox, result);
    response = format_search_result(result);
    
    if (response) {
        send(client->sock, response, strlen(response), MSG_NOSIGNAL);
        send(client->sock, "OK SEARCH completed\r\n", 21, MSG_NOSIGNAL);
        free(response);
    }
    
    search_free_result(result);
    return 0;
}

static int handle_idle(client_t *client, const char *args, mailbox_t *mailbox) {
    if (!client->authenticated) {
        send(client->sock, "NO not authenticated\r\n", 22, MSG_NOSIGNAL);
        return -1;
    }
    
    send(client->sock, "+ idling\r\n", 10, MSG_NOSIGNAL);
    
    client->idle_mode = 1;
    client->idle_start = time(NULL);
    
    return 0;
}

static int handle_logout(client_t *client, const char *args, mailbox_t *mailbox) {
    send(client->sock, "* BYE server shutting down\r\nOK LOGOUT completed\r\n", 50, MSG_NOSIGNAL);
    client->authenticated = 0;
    return 0;
}

static command_handler_t handlers[] = {
    {"CAPABILITY", handle_capability},
    {"LOGIN", handle_login},
    {"SELECT", handle_select},
    {"FETCH", handle_fetch},
    {"SEARCH", handle_search},
    {"IDLE", handle_idle},
    {"LOGOUT", handle_logout},
    {NULL, NULL}
};

int protocol_execute_command(client_t *client, const char *tag, const char *command, 
                            const char *args, mailbox_t *mailbox) {
    int i = 0;
    char response[256];
    
    if (!client || !tag || !command) return -1;
    
    pthread_rwlock_rdlock(&proto_lock);
    
    while (handlers[i].command != NULL) {
        if (strcasecmp(handlers[i].command, command) == 0) {
            pthread_rwlock_unlock(&proto_lock);
            
            int result = handlers[i].handler(client, args, mailbox);
            
            if (result == 0) {
                snprintf(response, sizeof(response), "%s OK %s completed\r\n", tag, command);
            } else {
                snprintf(response, sizeof(response), "%s NO %s failed\r\n", tag, command);
            }
            
            send(client->sock, response, strlen(response), MSG_NOSIGNAL);
            return result;
        }
        i++;
    }
    
    pthread_rwlock_unlock(&proto_lock);
    
    snprintf(response, sizeof(response), "%s BAD unknown command\r\n", tag);
    send(client->sock, response, strlen(response), MSG_NOSIGNAL);
    return -1;
}
