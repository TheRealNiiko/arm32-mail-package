#include "nexus_mp.h"
#include <stdarg.h>

#define VERSION "1.0.0"
#define BANNER "Nexus Mail Protocol Server v" VERSION " ready"

static server_config_t global_server;
static client_t client_pool[MAX_CLIENTS];
static pthread_mutex_t client_pool_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) {
    server_config_t config;
    const char *config_file = "/etc/nexus-mp/nexus-mp.conf";
    
    if (argc > 1) {
        config_file = argv[1];
    }
    
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    
    initialize_server(&config, config_file);
    start_server(&config);
    
    return 0;
}

void initialize_server(server_config_t *config, const char *config_file) {
    memset(config, 0, sizeof(server_config_t));
    
    config->port = NEXUS_MP_PORT;
    config->ssl_port = NEXUS_MP_SSL_PORT;
    config->max_connections = MAX_CLIENTS;
    config->use_ssl = 0;
    config->enable_syslog = 1;
    
    strncpy(config->mailstore_base, "/var/mail", sizeof(config->mailstore_base) - 1);
    strncpy(config->config_path, config_file, sizeof(config->config_path) - 1);
    
    if (access(config_file, R_OK) == 0) {
        FILE *f = fopen(config_file, "r");
        if (f) {
            char line[512];
            while (fgets(line, sizeof(line), f)) {
                char *key, *value;
                if (line[0] == '#' || line[0] == '\n') continue;
                
                key = strtok(line, "=");
                value = strtok(NULL, "\n");
                
                if (!key || !value) continue;
                
                key = strrchr(key, ' ') ? strrchr(key, ' ') + 1 : key;
                value = strchr(value, ' ') ? strchr(value, ' ') + 1 : value;
                
                if (strcmp(key, "port") == 0) {
                    config->port = atoi(value);
                } else if (strcmp(key, "ssl_port") == 0) {
                    config->ssl_port = atoi(value);
                } else if (strcmp(key, "mailstore") == 0) {
                    strncpy(config->mailstore_base, value, sizeof(config->mailstore_base) - 1);
                } else if (strcmp(key, "use_ssl") == 0) {
                    config->use_ssl = atoi(value);
                } else if (strcmp(key, "syslog") == 0) {
                    config->enable_syslog = atoi(value);
                }
            }
            fclose(f);
        }
    }
    
    if (config->enable_syslog) {
        openlog("nexus-mp", LOG_PID | LOG_CONS, LOG_MAIL);
    }
    
    memset(client_pool, 0, sizeof(client_pool));
    memcpy(&global_server, config, sizeof(server_config_t));
    
    log_event("INFO", "Nexus MP server initialized on port %d", config->port);
}

void start_server(server_config_t *config) {
    struct sockaddr_in server_addr;
    int opt = 1;
    int i;
    
    config->server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (config->server_sock < 0) {
        log_event("ERROR", "Failed to create socket: %s", strerror(errno));
        exit(1);
    }
    
    if (setsockopt(config->server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log_event("ERROR", "Failed to set socket options: %s", strerror(errno));
        close(config->server_sock);
        exit(1);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(config->port);
    
    if (bind(config->server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        log_event("ERROR", "Failed to bind socket: %s", strerror(errno));
        close(config->server_sock);
        exit(1);
    }
    
    if (listen(config->server_sock, 128) < 0) {
        log_event("ERROR", "Failed to listen on socket: %s", strerror(errno));
        close(config->server_sock);
        exit(1);
    }
    
    log_event("INFO", "Nexus MP server listening on port %d", config->port);
    
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_pool[i].sock = -1;
    }
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_sock;
        int client_idx = -1;
        
        client_sock = accept(config->server_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            if (errno != EINTR) {
                log_event("ERROR", "Accept failed: %s", strerror(errno));
            }
            continue;
        }
        
        pthread_mutex_lock(&client_pool_mutex);
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_pool[i].sock < 0) {
                client_idx = i;
                break;
            }
        }
        pthread_mutex_unlock(&client_pool_mutex);
        
        if (client_idx < 0) {
            log_event("WARNING", "Max clients reached, rejecting connection");
            close(client_sock);
            continue;
        }
        
        client_pool[client_idx].sock = client_sock;
        client_pool[client_idx].state = STATE_NOT_AUTHENTICATED;
        client_pool[client_idx].selected_mailbox = NULL;
        client_pool[client_idx].tag_counter = 0;
        client_pool[client_idx].last_activity = time(NULL);
        memcpy(&client_pool[client_idx].client_addr, &client_addr, sizeof(client_addr));
        
        pthread_create(&client_pool[client_idx].thread_id, NULL, handle_client, &client_pool[client_idx]);
        pthread_detach(client_pool[client_idx].thread_id);
    }
}

void shutdown_server(server_config_t *config) {
    log_event("INFO", "Shutting down Nexus MP server");
    if (config->server_sock >= 0) {
        close(config->server_sock);
    }
    if (config->enable_syslog) {
        closelog();
    }
}

void *handle_client(void *arg) {
    client_t *client = (client_t *)arg;
    char buffer[MAX_BUFFER];
    int bytes;
    
    send_untagged_response(client, "* OK " BANNER);
    
    while (1) {
        memset(buffer, 0, MAX_BUFFER);
        
        bytes = recv(client->sock, buffer, MAX_BUFFER - 1, 0);
        if (bytes <= 0) {
            break;
        }
        
        client->last_activity = time(NULL);
        
        char *line = strtok(buffer, "\r\n");
        while (line) {
            if (strlen(line) > 0) {
                process_command(client, line);
            }
            line = strtok(NULL, "\r\n");
        }
        
        if (client->state == STATE_LOGOUT) {
            break;
        }
    }
    
    close(client->sock);
    client->sock = -1;
    client->state = STATE_LOGOUT;
    
    return NULL;
}

void process_command(client_t *client, const char *command) {
    char tag[64];
    char cmd[128];
    char args[MAX_BUFFER];
    
    if (parse_imap_command(command, tag, cmd, args) < 0) {
        send_untagged_response(client, "* BAD Invalid command syntax");
        return;
    }
    
    if (strcmp(cmd, "CAPABILITY") == 0) {
        handle_capability(client, tag);
    } else if (strcmp(cmd, "NOOP") == 0) {
        send_tagged_response(client, tag, "OK", "NOOP completed");
    } else if (strcmp(cmd, "LOGOUT") == 0) {
        handle_logout(client, tag);
    } else if (strcmp(cmd, "LOGIN") == 0) {
        if (client->state != STATE_NOT_AUTHENTICATED) {
            send_tagged_response(client, tag, "NO", "Already authenticated");
        } else {
            handle_login(client, tag, args);
        }
    } else if (client->state == STATE_NOT_AUTHENTICATED) {
        send_tagged_response(client, tag, "NO", "Please authenticate first");
    } else if (strcmp(cmd, "SELECT") == 0) {
        handle_select(client, tag, args);
    } else if (strcmp(cmd, "EXAMINE") == 0) {
        handle_examine(client, tag, args);
    } else if (strcmp(cmd, "CREATE") == 0) {
        handle_create(client, tag, args);
    } else if (strcmp(cmd, "DELETE") == 0) {
        handle_delete(client, tag, args);
    } else if (strcmp(cmd, "RENAME") == 0) {
        handle_rename(client, tag, args);
    } else if (strcmp(cmd, "LIST") == 0) {
        handle_list(client, tag, args);
    } else if (strcmp(cmd, "LSUB") == 0) {
        handle_lsub(client, tag, args);
    } else if (strcmp(cmd, "STATUS") == 0) {
        handle_status(client, tag, args);
    } else if (strcmp(cmd, "FETCH") == 0) {
        handle_fetch(client, tag, args);
    } else if (strcmp(cmd, "STORE") == 0) {
        handle_store(client, tag, args);
    } else if (strcmp(cmd, "COPY") == 0) {
        handle_copy(client, tag, args);
    } else if (strcmp(cmd, "SEARCH") == 0) {
        handle_search(client, tag, args);
    } else if (strcmp(cmd, "APPEND") == 0) {
        handle_append(client, tag, args);
    } else if (strcmp(cmd, "CLOSE") == 0) {
        handle_close(client, tag);
    } else if (strcmp(cmd, "EXPUNGE") == 0) {
        handle_expunge(client, tag);
    } else {
        send_tagged_response(client, tag, "BAD", "Unknown command");
    }
}

void send_response(client_t *client, const char *response) {
    if (client->sock < 0) return;
    send(client->sock, response, strlen(response), 0);
    send(client->sock, "\r\n", 2, 0);
}

void send_tagged_response(client_t *client, const char *tag, const char *status, const char *message) {
    char buffer[MAX_BUFFER];
    snprintf(buffer, sizeof(buffer), "%s %s %s", tag, status, message);
    send_response(client, buffer);
}

void send_untagged_response(client_t *client, const char *response) {
    send_response(client, response);
}

int parse_imap_command(const char *command, char *tag, char *cmd, char *args) {
    int pos = 0;
    int tag_pos = 0;
    int cmd_pos = 0;
    
    if (!command || !tag || !cmd || !args) return -1;
    
    memset(tag, 0, 64);
    memset(cmd, 0, 128);
    memset(args, 0, MAX_BUFFER);
    
    while (command[pos] && command[pos] != ' ') {
        if (tag_pos < 63) tag[tag_pos++] = command[pos];
        pos++;
    }
    
    if (!command[pos]) return -1;
    pos++;
    
    while (command[pos] && command[pos] != ' ') {
        if (cmd_pos < 127) cmd[cmd_pos++] = command[pos];
        pos++;
    }
    
    if (command[pos] == ' ') {
        pos++;
        strncpy(args, &command[pos], MAX_BUFFER - 1);
    }
    
    return 0;
}

void handle_capability(client_t *client, const char *tag) {
    send_untagged_response(client, "* CAPABILITY IMAP4rev1 LITERAL+ SASL-IR LOGIN-REFERRALS ID ENABLE IDLE NAMESPACE CHILDREN X-FORWARD X-VENDOR");
    send_tagged_response(client, tag, "OK", "CAPABILITY completed");
}

void handle_login(client_t *client, const char *tag, const char *args) {
    char username[128] = {0};
    char password[256] = {0};
    user_t user;
    
    if (sscanf(args, "%127s %255s", username, password) != 2) {
        send_tagged_response(client, tag, "BAD", "Invalid LOGIN syntax");
        return;
    }
    
    if (authenticate_user(username, password, &user) == 0) {
        strncpy(client->username, username, sizeof(client->username) - 1);
        client->state = STATE_AUTHENTICATED;
        send_tagged_response(client, tag, "OK", "LOGIN completed");
        log_event("INFO", "User %s authenticated from %s", username, inet_ntoa(client->client_addr.sin_addr));
    } else {
        send_tagged_response(client, tag, "NO", "Invalid authentication credentials");
        log_event("WARNING", "Failed login attempt for %s from %s", username, inet_ntoa(client->client_addr.sin_addr));
    }
}

void handle_logout(client_t *client, const char *tag) {
    send_untagged_response(client, "* BYE Nexus MP server closing connection");
    send_tagged_response(client, tag, "OK", "LOGOUT completed");
    client->state = STATE_LOGOUT;
}

void handle_select(client_t *client, const char *tag, const char *mailbox) {
    if (client->state != STATE_AUTHENTICATED && client->state != STATE_SELECTED) {
        send_tagged_response(client, tag, "NO", "Not authenticated");
        return;
    }
    
    if (select_mailbox(client, mailbox, 0) == 0) {
        client->state = STATE_SELECTED;
        send_untagged_response(client, "* FLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft)");
        send_untagged_response(client, "* OK [PERMANENTFLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft \\*)]");
        send_tagged_response(client, tag, "OK", "[READ-WRITE] SELECT completed");
    } else {
        send_tagged_response(client, tag, "NO", "Mailbox does not exist");
    }
}

void handle_examine(client_t *client, const char *tag, const char *mailbox) {
    if (client->state != STATE_AUTHENTICATED && client->state != STATE_SELECTED) {
        send_tagged_response(client, tag, "NO", "Not authenticated");
        return;
    }
    
    if (select_mailbox(client, mailbox, 1) == 0) {
        client->state = STATE_SELECTED;
        send_untagged_response(client, "* FLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft)");
        send_untagged_response(client, "* OK [PERMANENTFLAGS ()]");
        send_tagged_response(client, tag, "OK", "[READ-ONLY] EXAMINE completed");
    } else {
        send_tagged_response(client, tag, "NO", "Mailbox does not exist");
    }
}

void handle_create(client_t *client, const char *tag, const char *mailbox) {
    char mailbox_path[512];
    
    if (client->state < STATE_AUTHENTICATED) {
        send_tagged_response(client, tag, "NO", "Not authenticated");
        return;
    }
    
    snprintf(mailbox_path, sizeof(mailbox_path), "%s/%s/.nexus-mp-mailbox", global_server.mailstore_base, client->username, mailbox);
    
    if (mkdir(mailbox_path, 0700) == 0) {
        send_tagged_response(client, tag, "OK", "CREATE completed");
    } else {
        send_tagged_response(client, tag, "NO", "Failed to create mailbox");
    }
}

void handle_delete(client_t *client, const char *tag, const char *mailbox) {
    send_tagged_response(client, tag, "OK", "DELETE completed");
}

void handle_rename(client_t *client, const char *tag, const char *args) {
    send_tagged_response(client, tag, "OK", "RENAME completed");
}

void handle_list(client_t *client, const char *tag, const char *args) {
    send_untagged_response(client, "* LIST (\\Noselect) \"/\" \"\"");
    send_untagged_response(client, "* LIST (\\HasNoChildren) \"/\" \"INBOX\"");
    send_tagged_response(client, tag, "OK", "LIST completed");
}

void handle_lsub(client_t *client, const char *tag, const char *args) {
    send_untagged_response(client, "* LSUB () \"/\" \"\"");
    send_tagged_response(client, tag, "OK", "LSUB completed");
}

void handle_status(client_t *client, const char *tag, const char *args) {
    send_untagged_response(client, "* STATUS INBOX (MESSAGES 0 RECENT 0 UNSEEN 0)");
    send_tagged_response(client, tag, "OK", "STATUS completed");
}

void handle_fetch(client_t *client, const char *tag, const char *args) {
    send_tagged_response(client, tag, "OK", "FETCH completed");
}

void handle_store(client_t *client, const char *tag, const char *args) {
    send_tagged_response(client, tag, "OK", "STORE completed");
}

void handle_copy(client_t *client, const char *tag, const char *args) {
    send_tagged_response(client, tag, "OK", "COPY completed");
}

void handle_search(client_t *client, const char *tag, const char *args) {
    send_untagged_response(client, "* SEARCH");
    send_tagged_response(client, tag, "OK", "SEARCH completed");
}

void handle_append(client_t *client, const char *tag, const char *args) {
    send_tagged_response(client, tag, "OK", "APPEND completed");
}

void handle_close(client_t *client, const char *tag) {
    client->selected_mailbox = NULL;
    client->state = STATE_AUTHENTICATED;
    send_tagged_response(client, tag, "OK", "CLOSE completed");
}

void handle_expunge(client_t *client, const char *tag) {
    send_tagged_response(client, tag, "OK", "EXPUNGE completed");
}

int authenticate_user(const char *username, const char *password, user_t *user) {
    FILE *passwd_file;
    char line[512];
    char uname[128];
    char pwhash[256];
    
    passwd_file = fopen("/etc/nexus-mp/passwd", "r");
    if (!passwd_file) {
        return -1;
    }
    
    while (fgets(line, sizeof(line), passwd_file)) {
        if (sscanf(line, "%127[^:]:%255s", uname, pwhash) == 2) {
            if (strcmp(uname, username) == 0) {
                if (strcmp(pwhash, password) == 0) {
                    fclose(passwd_file);
                    strncpy(user->username, username, sizeof(user->username) - 1);
                    return 0;
                }
            }
        }
    }
    
    fclose(passwd_file);
    return -1;
}

int create_user_maildir(const char *username, const char *base_path) {
    char maildir[512];
    snprintf(maildir, sizeof(maildir), "%s/%s", base_path, username);
    if (mkdir(maildir, 0700) < 0 && errno != EEXIST) {
        return -1;
    }
    return 0;
}

int select_mailbox(client_t *client, const char *mailbox_name, int readonly) {
    char mailbox_path[512];
    DIR *dir;
    
    snprintf(mailbox_path, sizeof(mailbox_path), "%s/%s/%s", global_server.mailstore_base, client->username, mailbox_name);
    
    dir = opendir(mailbox_path);
    if (!dir) {
        return -1;
    }
    
    closedir(dir);
    return 0;
}

void log_event(const char *level, const char *format, ...) {
    va_list args;
    char buffer[1024];
    time_t now;
    struct tm *timeinfo;
    char timestamp[32];
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    if (global_server.enable_syslog) {
        if (strcmp(level, "ERROR") == 0) {
            syslog(LOG_ERR, "%s", buffer);
        } else if (strcmp(level, "WARNING") == 0) {
            syslog(LOG_WARNING, "%s", buffer);
        } else {
            syslog(LOG_INFO, "%s", buffer);
        }
    }
    
    fprintf(stderr, "[%s] %s: %s\n", timestamp, level, buffer);
}

void signal_handler(int sig) {
    log_event("INFO", "Received signal %d", sig);
    shutdown_server(&global_server);
    exit(0);
}
