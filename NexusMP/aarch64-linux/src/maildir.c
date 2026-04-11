#include "maildir.h"
#include <openssl/md5.h>

int maildir_init(const char *username, const char *maildir_path) {
    char inbox_path[512];
    char cur_path[512];
    char new_path[512];
    char tmp_path[512];
    
    if (mkdir(maildir_path, 0700) < 0 && errno != EEXIST) {
        return -1;
    }
    
    snprintf(inbox_path, sizeof(inbox_path), "%s/INBOX", maildir_path);
    snprintf(cur_path, sizeof(cur_path), "%s/cur", inbox_path);
    snprintf(new_path, sizeof(new_path), "%s/new", inbox_path);
    snprintf(tmp_path, sizeof(tmp_path), "%s/tmp", inbox_path);
    
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
    
    return 0;
}

int maildir_list_messages(const char *maildir_path, message_t *messages, int max_messages) {
    DIR *dir;
    struct dirent *entry;
    char cur_path[512];
    int count = 0;
    
    snprintf(cur_path, sizeof(cur_path), "%s/INBOX/cur", maildir_path);
    
    dir = opendir(cur_path);
    if (!dir) {
        return 0;
    }
    
    while ((entry = readdir(dir)) && count < max_messages) {
        if (entry->d_type == DT_REG && strchr(entry->d_name, ':')) {
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", cur_path, entry->d_name);
            
            if (read_message_headers(full_path, &messages[count]) == 0) {
                messages[count].uid = calculate_message_uid(entry->d_name);
                messages[count].seq = count + 1;
                count++;
            }
        }
    }
    
    closedir(dir);
    return count;
}

int maildir_get_message(const char *maildir_path, uint32_t uid, message_t *msg) {
    DIR *dir;
    struct dirent *entry;
    char cur_path[512];
    
    snprintf(cur_path, sizeof(cur_path), "%s/INBOX/cur", maildir_path);
    
    dir = opendir(cur_path);
    if (!dir) {
        return -1;
    }
    
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_REG && calculate_message_uid(entry->d_name) == uid) {
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", cur_path, entry->d_name);
            closedir(dir);
            return read_message_headers(full_path, msg);
        }
    }
    
    closedir(dir);
    return -1;
}

int maildir_add_message(const char *maildir_path, const char *message_data, uint32_t size) {
    FILE *f;
    char tmp_path[512];
    char cur_path[512];
    char new_filename[256];
    time_t now;
    
    time(&now);
    snprintf(new_filename, sizeof(new_filename), "%ld.%u.localhost,S=%u", now, rand(), size);
    snprintf(tmp_path, sizeof(tmp_path), "%s/INBOX/tmp/%s", maildir_path, new_filename);
    
    f = fopen(tmp_path, "w");
    if (!f) {
        return -1;
    }
    
    if (fwrite(message_data, 1, size, f) != size) {
        fclose(f);
        unlink(tmp_path);
        return -1;
    }
    
    fclose(f);
    
    snprintf(cur_path, sizeof(cur_path), "%s/INBOX/cur/%s:2,", maildir_path, new_filename);
    
    if (link(tmp_path, cur_path) < 0) {
        unlink(tmp_path);
        return -1;
    }
    
    unlink(tmp_path);
    return 0;
}

int maildir_delete_message(const char *maildir_path, uint32_t uid) {
    DIR *dir;
    struct dirent *entry;
    char cur_path[512];
    
    snprintf(cur_path, sizeof(cur_path), "%s/INBOX/cur", maildir_path);
    
    dir = opendir(cur_path);
    if (!dir) {
        return -1;
    }
    
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_REG && calculate_message_uid(entry->d_name) == uid) {
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", cur_path, entry->d_name);
            closedir(dir);
            return unlink(full_path);
        }
    }
    
    closedir(dir);
    return -1;
}

int maildir_set_flags(const char *maildir_path, uint32_t uid, uint32_t flags) {
    DIR *dir;
    struct dirent *entry;
    char cur_path[512];
    
    snprintf(cur_path, sizeof(cur_path), "%s/INBOX/cur", maildir_path);
    
    dir = opendir(cur_path);
    if (!dir) {
        return -1;
    }
    
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_REG && calculate_message_uid(entry->d_name) == uid) {
            char old_path[512];
            char new_path[512];
            char flag_str[32] = {0};
            
            snprintf(old_path, sizeof(old_path), "%s/%s", cur_path, entry->d_name);
            
            if (flags & FLAG_SEEN) strcat(flag_str, "S");
            if (flags & FLAG_ANSWERED) strcat(flag_str, "R");
            if (flags & FLAG_FLAGGED) strcat(flag_str, "F");
            if (flags & FLAG_DELETED) strcat(flag_str, "T");
            if (flags & FLAG_DRAFT) strcat(flag_str, "D");
            
            char *colon = strchr(entry->d_name, ':');
            if (colon) {
                *colon = '\0';
                snprintf(new_path, sizeof(new_path), "%s/%s:2,%s", cur_path, entry->d_name, flag_str);
            } else {
                snprintf(new_path, sizeof(new_path), "%s/%s:2,%s", cur_path, entry->d_name, flag_str);
            }
            
            closedir(dir);
            
            if (rename(old_path, new_path) == 0) {
                return 0;
            }
            return -1;
        }
    }
    
    closedir(dir);
    return -1;
}

int maildir_get_flags(const char *maildir_path, uint32_t uid) {
    DIR *dir;
    struct dirent *entry;
    char cur_path[512];
    uint32_t flags = 0;
    
    snprintf(cur_path, sizeof(cur_path), "%s/INBOX/cur", maildir_path);
    
    dir = opendir(cur_path);
    if (!dir) {
        return 0;
    }
    
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_REG && calculate_message_uid(entry->d_name) == uid) {
            char *flag_part = strchr(entry->d_name, ',');
            if (flag_part) {
                flag_part++; 
                flag_part++; 
                
                if (strchr(flag_part, 'S')) flags |= FLAG_SEEN;
                if (strchr(flag_part, 'R')) flags |= FLAG_ANSWERED;
                if (strchr(flag_part, 'F')) flags |= FLAG_FLAGGED;
                if (strchr(flag_part, 'T')) flags |= FLAG_DELETED;
                if (strchr(flag_part, 'D')) flags |= FLAG_DRAFT;
            }
            
            closedir(dir);
            return flags;
        }
    }
    
    closedir(dir);
    return 0;
}

int read_message_headers(const char *filename, message_t *msg) {
    FILE *f;
    char line[1024];
    struct stat st;
    
    if (stat(filename, &st) < 0) {
        return -1;
    }
    
    msg->size = st.st_size;
    msg->date = st.st_mtime;
    
    f = fopen(filename, "r");
    if (!f) {
        return -1;
    }
    
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\r') {
            break;
        }
        
        if (strncmp(line, "Subject:", 8) == 0) {
            strncpy(msg->subject, line + 9, sizeof(msg->subject) - 1);
        } else if (strncmp(line, "From:", 5) == 0) {
            strncpy(msg->from, line + 6, sizeof(msg->from) - 1);
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
    return uid;
}
