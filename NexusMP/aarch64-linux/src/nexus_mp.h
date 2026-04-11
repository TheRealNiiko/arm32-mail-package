#ifndef NEXUS_MP_H
#define NEXUS_MP_H

#include "pthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/rand.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <ctype.h>
#include <stdarg.h>
#include <assert.h>
#include <limits.h>
#include <wchar.h>

#define NEXUS_MP_VERSION "1.0.0"
#define NEXUS_MP_PORT 143
#define NEXUS_MP_SSL_PORT 993
#define NEXUS_MP_IDLE_PORT 143
#define MAX_CLIENTS 1024
#define MAX_BUFFER 16384
#define MAX_MAILBOXES 512
#define MAX_MESSAGES 131072
#define MAX_FLAGS 32
#define DEFAULT_TIMEOUT 300
#define READ_TIMEOUT 60
#define WRITE_TIMEOUT 30
#define COMMAND_TIMEOUT 600
#define MAX_LITERAL_SIZE 52428800
#define MAX_COMMAND_SIZE 8192
#define CACHE_SIZE 10485760
#define MAX_CONNECTIONS_PER_IP 32
#define THREAD_POOL_SIZE 64
#define EPOLL_MAX_EVENTS 256
#define BUFFER_POOL_SIZE 512

typedef enum {
    STATE_NOT_AUTHENTICATED,
    STATE_AUTHENTICATED,
    STATE_SELECTED,
    STATE_LOGOUT
} imap_state_t;

typedef enum {
    FLAG_SEEN = 1,
    FLAG_ANSWERED = 2,
    FLAG_FLAGGED = 4,
    FLAG_DELETED = 8,
    FLAG_DRAFT = 16,
    FLAG_RECENT = 32
} message_flag_t;

typedef struct {
    uint32_t uid;
    uint32_t seq;
    time_t date;
    uint32_t size;
    uint32_t flags;
    char subject[512];
    char from[256];
    char envelope[2048];
} message_t;

typedef struct {
    char name[128];
    char path[512];
    uint32_t message_count;
    uint32_t recent_count;
    uint32_t unseen_count;
    message_t messages[MAX_MESSAGES];
    uint32_t uidnext;
    uint32_t uidvalidity;
    int is_selectable;
} mailbox_t;

typedef struct {
    int sock;
    char username[128];
    char password[256];
    imap_state_t state;
    mailbox_t *selected_mailbox;
    uint32_t tag_counter;
    time_t last_activity;
    struct sockaddr_in client_addr;
    pthread_t thread_id;
} client_t;

typedef struct {
    char username[128];
    char password_hash[256];
    char maildir[512];
    uint32_t uid;
    uint32_t gid;
    int active;
} user_t;

typedef struct {
    int server_sock;
    int ssl_server_sock;
    in_port_t port;
    in_port_t ssl_port;
    int max_connections;
    char config_path[512];
    char mailstore_base[512];
    int use_ssl;
    int enable_syslog;
} server_config_t;

void initialize_server(server_config_t *config, const char *config_file);
void start_server(server_config_t *config);
void shutdown_server(server_config_t *config);

void *handle_client(void *arg);
void process_command(client_t *client, const char *command);
void send_response(client_t *client, const char *response);
void send_tagged_response(client_t *client, const char *tag, const char *status, const char *message);
void send_untagged_response(client_t *client, const char *response);

int authenticate_user(const char *username, const char *password, user_t *user);
int create_user_maildir(const char *username, const char *base_path);

int load_mailbox(client_t *client, const char *mailbox_name);
int select_mailbox(client_t *client, const char *mailbox_name, int readonly);
int list_mailboxes(client_t *client, const char *reference, const char *pattern);
int mailbox_status(client_t *client, const char *mailbox_name);

int fetch_message(client_t *client, uint32_t seq, const char *items);
int store_flags(client_t *client, uint32_t seq, const char *flags, int silent);
int search_messages(client_t *client, const char *criteria);

int parse_imap_command(const char *command, char *tag, char *cmd, char *args);
int parse_search_criteria(const char *criteria, char **conditions);

void handle_capability(client_t *client, const char *tag);
void handle_login(client_t *client, const char *tag, const char *args);
void handle_logout(client_t *client, const char *tag);
void handle_select(client_t *client, const char *tag, const char *mailbox);
void handle_examine(client_t *client, const char *tag, const char *mailbox);
void handle_create(client_t *client, const char *tag, const char *mailbox);
void handle_delete(client_t *client, const char *tag, const char *mailbox);
void handle_rename(client_t *client, const char *tag, const char *args);
void handle_list(client_t *client, const char *tag, const char *args);
void handle_lsub(client_t *client, const char *tag, const char *args);
void handle_status(client_t *client, const char *tag, const char *args);
void handle_fetch(client_t *client, const char *tag, const char *args);
void handle_store(client_t *client, const char *tag, const char *args);
void handle_copy(client_t *client, const char *tag, const char *args);
void handle_search(client_t *client, const char *tag, const char *args);
void handle_append(client_t *client, const char *tag, const char *args);
void handle_close(client_t *client, const char *tag);
void handle_expunge(client_t *client, const char *tag);

int parse_message_date(const char *date_str);
char *get_message_envelope(const char *message_path);
uint32_t calculate_message_uid(const char *filename);

void log_event(const char *level, const char *format, ...);
void signal_handler(int sig);

#endif
