#include "config.h"
#include <ctype.h>

static pthread_rwlock_t config_lock = PTHREAD_RWLOCK_INITIALIZER;

config_t *config_create(void) {
    config_t *cfg = malloc(sizeof(config_t));
    if (!cfg) return NULL;
    
    cfg->entries = malloc(100 * sizeof(config_entry_t));
    if (!cfg->entries) {
        free(cfg);
        return NULL;
    }
    
    cfg->entry_count = 0;
    cfg->entry_capacity = 100;
    
    return cfg;
}

void config_free(config_t *cfg) {
    if (cfg) {
        for (unsigned int i = 0; i < cfg->entry_count; i++) {
            free(cfg->entries[i].key);
            free(cfg->entries[i].value);
        }
        free(cfg->entries);
        free(cfg);
    }
}

static char *config_expand_variables(const char *value) {
    char *expanded, *pos, *var_start, *var_end, *var_value;
    char var_name[256];
    size_t len;
    
    if (!value || strchr(value, '$') == NULL) {
        return strdup(value ? value : "");
    }
    
    expanded = malloc(4096);
    if (!expanded) return NULL;
    
    pos = expanded;
    var_start = (char *)value;
    
    while (*var_start) {
        var_end = strchr(var_start, '$');
        if (!var_end) {
            strcpy(pos, var_start);
            break;
        }
        
        len = var_end - var_start;
        strncpy(pos, var_start, len);
        pos += len;
        
        if (*(var_end + 1) == '{') {
            var_end += 2;
            char *close = strchr(var_end, '}');
            if (close) {
                strncpy(var_name, var_end, close - var_end);
                var_name[close - var_end] = '\0';
                var_value = getenv(var_name);
                if (var_value) {
                    strcpy(pos, var_value);
                    pos += strlen(var_value);
                }
                var_start = close + 1;
            }
        } else {
            var_start = var_end + 1;
        }
    }
    
    *pos = '\0';
    return expanded;
}

static int config_create_directories(const char *path) {
    char *dir_path = strdup(path);
    if (!dir_path) return -1;
    
    for (char *p = dir_path + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(dir_path, 0755);
            *p = '/';
        }
    }
    
    free(dir_path);
    return 0;
}

int config_load_file(config_t *cfg, const char *filename) {
    FILE *fp;
    char line[1024];
    char *key, *value, *eq;
    config_entry_t *new_entries;
    
    if (!cfg || !filename) return -1;
    
    fp = fopen(filename, "r");
    if (!fp) return -1;
    
    pthread_rwlock_wrlock(&config_lock);
    
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = '\0';
        
        if (line[0] == '\0' || line[0] == '#') continue;
        
        eq = strchr(line, '=');
        if (!eq) continue;
        
        *eq = '\0';
        key = line;
        value = eq + 1;
        
        while (*key && isspace(*key)) key++;
        while (key[strlen(key) - 1] && isspace(key[strlen(key) - 1])) {
            key[strlen(key) - 1] = '\0';
        }
        
        while (*value && isspace(*value)) value++;
        while (value[strlen(value) - 1] && isspace(value[strlen(value) - 1])) {
            value[strlen(value) - 1] = '\0';
        }
        
        if (cfg->entry_count >= cfg->entry_capacity) {
            cfg->entry_capacity *= 2;
            new_entries = realloc(cfg->entries, cfg->entry_capacity * sizeof(config_entry_t));
            if (!new_entries) {
                fclose(fp);
                pthread_rwlock_unlock(&config_lock);
                return -1;
            }
            cfg->entries = new_entries;
        }
        
        cfg->entries[cfg->entry_count].key = strdup(key);
        cfg->entries[cfg->entry_count].value = config_expand_variables(value);
        cfg->entry_count++;
    }
    
    fclose(fp);
    pthread_rwlock_unlock(&config_lock);
    
    return 0;
}

const char *config_get_string(const config_t *cfg, const char *key, const char *default_value) {
    if (!cfg || !key) return default_value;
    
    pthread_rwlock_rdlock(&config_lock);
    
    for (unsigned int i = 0; i < cfg->entry_count; i++) {
        if (strcmp(cfg->entries[i].key, key) == 0) {
            pthread_rwlock_unlock(&config_lock);
            return cfg->entries[i].value;
        }
    }
    
    pthread_rwlock_unlock(&config_lock);
    return default_value;
}

int config_get_int(const config_t *cfg, const char *key, int default_value) {
    const char *value = config_get_string(cfg, key, NULL);
    if (!value) return default_value;
    return atoi(value);
}

int config_get_bool(const config_t *cfg, const char *key, int default_value) {
    const char *value = config_get_string(cfg, key, NULL);
    if (!value) return default_value;
    
    return (strcmp(value, "true") == 0 || strcmp(value, "yes") == 0 || 
            strcmp(value, "1") == 0) ? 1 : 0;
}

int config_set_value(config_t *cfg, const char *key, const char *value) {
    if (!cfg || !key || !value) return -1;
    
    pthread_rwlock_wrlock(&config_lock);
    
    for (unsigned int i = 0; i < cfg->entry_count; i++) {
        if (strcmp(cfg->entries[i].key, key) == 0) {
            free(cfg->entries[i].value);
            cfg->entries[i].value = strdup(value);
            pthread_rwlock_unlock(&config_lock);
            return 0;
        }
    }
    
    if (cfg->entry_count >= cfg->entry_capacity) {
        cfg->entry_capacity *= 2;
        config_entry_t *new_entries = realloc(cfg->entries, cfg->entry_capacity * sizeof(config_entry_t));
        if (!new_entries) {
            pthread_rwlock_unlock(&config_lock);
            return -1;
        }
        cfg->entries = new_entries;
    }
    
    cfg->entries[cfg->entry_count].key = strdup(key);
    cfg->entries[cfg->entry_count].value = strdup(value);
    cfg->entry_count++;
    
    pthread_rwlock_unlock(&config_lock);
    return 0;
}

char *config_get_directory(const char *type) {
    char *dir = NULL;
    const char *base_dir = getenv("NEXUS_MP_HOME");
    
    if (!base_dir) {
        base_dir = "/var/lib/nexus-mp";
    }
    
    if (strcmp(type, "maildir") == 0) {
        asprintf(&dir, "%s/mail", base_dir);
    } else if (strcmp(type, "data") == 0) {
        asprintf(&dir, "%s/data", base_dir);
    } else if (strcmp(type, "tmp") == 0) {
        asprintf(&dir, "%s/tmp", base_dir);
    } else if (strcmp(type, "cache") == 0) {
        asprintf(&dir, "%s/cache", base_dir);
    } else if (strcmp(type, "log") == 0) {
        asprintf(&dir, "%s/log", base_dir);
    }
    
    if (dir) {
        config_create_directories(dir);
    }
    
    return dir;
}
