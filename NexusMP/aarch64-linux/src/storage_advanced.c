#include "storage.h"

static storage_backend_t current_backend = STORAGE_MAILDIR;
static pthread_rwlock_t storage_lock = PTHREAD_RWLOCK_INITIALIZER;

int storage_init(const char *base_path, storage_backend_t backend) {
    pthread_rwlock_init(&storage_lock, NULL);
    
    if (mkdir(base_path, 0755) < 0 && errno != EEXIST) {
        return -1;
    }
    
    current_backend = backend;
    return 0;
}

int storage_cleanup(void) {
    pthread_rwlock_destroy(&storage_lock);
    return 0;
}

int maildir_init(const char *username, const char *maildir_path) {
    char inbox_path[1024];
    char cur_path[1024];
    char new_path[1024];
    char tmp_path[1024];
    char dottimes_path[1024];
    FILE *dottimes_file;
    
    if (mkdir(maildir_path, 0700) < 0 && errno != EEXIST) {
        return -1;
    }
    
    snprintf(inbox_path, sizeof(inbox_path), "%s/INBOX", maildir_path);
    snprintf(cur_path, sizeof(cur_path), "%s/cur", inbox_path);
    snprintf(new_path, sizeof(new_path), "%s/new", inbox_path);
    snprintf(tmp_path, sizeof(tmp_path), "%s/tmp", inbox_path);
    snprintf(dottimes_path, sizeof(dottimes_path), "%s/.mtime", inbox_path);
    
    if (mkdir(inbox_path, 0700) < 0 && errno != EEXIST) {
        return -1;
    }
    
    if (mkdir(cur_path, 0700) < 0 && errno != EEXIST) {
        return -1;
    }
    
    if (mkdir(new_path, 0700) < 0 && errno != EEXIST) {
        return -1;
    }
    
    if (mkdir(tmp_path, 0700) < 0 && errno != EEXIST) {
        return -1;
    }
    
    dottimes_file = fopen(dottimes_path, "w");
    if (dottimes_file) {
        fprintf(dottimes_file, "%ld\n", time(NULL));
        fclose(dottimes_file);
    }
    
    return 0;
}

int maildir_list_messages(const char *maildir_path, message_t **messages, uint32_t *count) {
    DIR *dir;
    struct dirent *entry;
    char cur_path[1024];
    int msg_count = 0;
    int capacity = 256;
    message_t *msg_array;
    
    snprintf(cur_path, sizeof(cur_path), "%s/INBOX/cur", maildir_path);
    
    dir = opendir(cur_path);
    if (!dir) {
        return -1;
    }
    
    msg_array = malloc(capacity * sizeof(message_t));
    if (!msg_array) {
        closedir(dir);
        return -1;
    }
    
    while ((entry = readdir(dir)) && msg_count < 131072) {
        if (entry->d_type == DT_REG && strchr(entry->d_name, ':')) {
            char full_path[1024];
            
            if (msg_count >= capacity) {
                capacity *= 2;
                message_t *new_array = realloc(msg_array, capacity * sizeof(message_t));
                if (!new_array) {
                    free(msg_array);
                    closedir(dir);
                    return -1;
                }
                msg_array = new_array;
            }
            
            snprintf(full_path, sizeof(full_path), "%s/%s", cur_path, entry->d_name);
            
            if (read_message_headers(full_path, &msg_array[msg_count]) == 0) {
                msg_array[msg_count].uid = calculate_message_uid(entry->d_name);
                msg_array[msg_count].seq = msg_count + 1;
                msg_count++;
            }
        }
    }
    
    closedir(dir);
    
    *messages = msg_array;
    *count = msg_count;
    
    return 0;
}

int read_message_headers(const char *filename, message_t *msg) {
    FILE *f;
    char line[4096];
    struct stat st;
    size_t header_size;
    
    if (stat(filename, &st) < 0) {
        return -1;
    }
    
    msg->size = st.st_size;
    msg->date = st.st_mtime;
    
    memset(msg->subject, 0, sizeof(msg->subject));
    memset(msg->from, 0, sizeof(msg->from));
    memset(msg->to, 0, sizeof(msg->to));
    
    f = fopen(filename, "r");
    if (!f) {
        return -1;
    }
    
    header_size = 0;
    while (fgets(line, sizeof(line), f) && header_size < 8192) {
        if (line[0] == '\n' || (line[0] == '\r' && line[1] == '\n')) {
            break;
        }
        
        header_size += strlen(line);
        
        if (strncmp(line, "Subject:", 8) == 0) {
            strncpy(msg->subject, line + 9, sizeof(msg->subject) - 1);
            char *nl = strchr(msg->subject, '\n');
            if (nl) *nl = '\0';
            nl = strchr(msg->subject, '\r');
            if (nl) *nl = '\0';
        } else if (strncmp(line, "From:", 5) == 0) {
            strncpy(msg->from, line + 6, sizeof(msg->from) - 1);
            char *nl = strchr(msg->from, '\n');
            if (nl) *nl = '\0';
            nl = strchr(msg->from, '\r');
            if (nl) *nl = '\0';
        } else if (strncmp(line, "To:", 3) == 0) {
            strncpy(msg->to, line + 4, sizeof(msg->to) - 1);
            char *nl = strchr(msg->to, '\n');
            if (nl) *nl = '\0';
            nl = strchr(msg->to, '\r');
            if (nl) *nl = '\0';
        } else if (strncmp(line, "Date:", 5) == 0) {
            parse_rfc2822_date(line + 6, &msg->received);
        } else if (strncmp(line, "Message-ID:", 11) == 0) {
            strncpy(msg->message_id, line + 12, sizeof(msg->message_id) - 1);
            char *nl = strchr(msg->message_id, '\n');
            if (nl) *nl = '\0';
            nl = strchr(msg->message_id, '\r');
            if (nl) *nl = '\0';
        } else if (strncmp(line, "In-Reply-To:", 12) == 0) {
            strncpy(msg->in_reply_to, line + 13, sizeof(msg->in_reply_to) - 1);
            char *nl = strchr(msg->in_reply_to, '\n');
            if (nl) *nl = '\0';
            nl = strchr(msg->in_reply_to, '\r');
            if (nl) *nl = '\0';
        } else if (strncmp(line, "References:", 11) == 0) {
            strncpy(msg->references, line + 12, sizeof(msg->references) - 1);
            char *nl = strchr(msg->references, '\n');
            if (nl) *nl = '\0';
            nl = strchr(msg->references, '\r');
            if (nl) *nl = '\0';
        } else if (strncmp(line, "Content-Type:", 13) == 0) {
            strncpy(msg->content_type, line + 14, sizeof(msg->content_type) - 1);
            char *nl = strchr(msg->content_type, '\n');
            if (nl) *nl = '\0';
            nl = strchr(msg->content_type, '\r');
            if (nl) *nl = '\0';
        } else if (strncmp(line, "Content-Transfer-Encoding:", 26) == 0) {
            strncpy(msg->content_transfer_encoding, line + 27, sizeof(msg->content_transfer_encoding) - 1);
            char *nl = strchr(msg->content_transfer_encoding, '\n');
            if (nl) *nl = '\0';
            nl = strchr(msg->content_transfer_encoding, '\r');
            if (nl) *nl = '\0';
        }
    }
    
    msg->body_lines = 0;
    while (fgets(line, sizeof(line), f)) {
        if (line[0] != '\0') {
            msg->body_lines++;
        }
    }
    
    fclose(f);
    return 0;
}

uint32_t calculate_message_uid(const char *filename) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5_CTX ctx;
    uint32_t uid;
    
    MD5_Init(&ctx);
    MD5_Update(&ctx, (unsigned char *)filename, strlen(filename));
    MD5_Final(digest, &ctx);
    
    memcpy(&uid, digest, sizeof(uint32_t));
    return uid ^ (uint32_t)time(NULL);
}

int parse_rfc2822_date(const char *date_str, time_t *timestamp) {
    struct tm tm;
    char *result;
    
    memset(&tm, 0, sizeof(tm));
    
    result = strptime(date_str, "%a, %d %b %Y %H:%M:%S %z", &tm);
    if (!result) {
        result = strptime(date_str, "%d %b %Y %H:%M:%S %z", &tm);
    }
    
    if (result) {
        *timestamp = mktime(&tm);
        return 0;
    }
    
    return -1;
}
