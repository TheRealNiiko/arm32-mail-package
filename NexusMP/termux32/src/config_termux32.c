#include "config_termux32.h"
#include <ctype.h>

config_t32_t *config_load_t32(const char *config_file) {
    if (!config_file) return NULL;

    config_t32_t *config = malloc(sizeof(config_t32_t));
    if (!config) return NULL;

    strncpy(config->config_file, config_file, 255);
    config->config_file[255] = '\0';

    /* ARM32: start with 32 entries (was 64) */
    config->entries = malloc(sizeof(config_entry_t32_t) * 32);
    if (!config->entries) {
        free(config);
        return NULL;
    }

    config->entry_count    = 0;
    config->entry_capacity = 32;
    pthread_rwlock_init(&config->lock, NULL);

    FILE *fp = fopen(config_file, "r");
    if (!fp)
        return config;  /* empty config is valid */

    /* ARM32: stack buffer matches CONFIG_MAX_VALUE */
    char line[CONFIG_MAX_VALUE];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;

        char *key   = line;
        char *value = eq + 1;
        *eq = '\0';

        while (key < eq && isspace((unsigned char)*key)) key++;
        while (eq > key && isspace((unsigned char)*(eq - 1))) *(eq - 1) = '\0';
        while (*value && isspace((unsigned char)*value)) value++;
        int vlen = (int)strlen(value);
        while (vlen > 0 && isspace((unsigned char)value[vlen - 1])) value[--vlen] = '\0';

        config_set_value_t32(config, key, value, CONF_STRING);
    }

    fclose(fp);
    return config;
}

void config_free_t32(config_t32_t *config) {
    if (!config) return;

    pthread_rwlock_destroy(&config->lock);
    free(config->entries);
    free(config);
}

const char *config_get_string_t32(config_t32_t *config, const char *key, const char *default_value) {
    if (!config || !key) return default_value;

    pthread_rwlock_rdlock(&config->lock);

    for (int i = 0; i < config->entry_count; i++) {
        if (strcmp(config->entries[i].key, key) == 0) {
            const char *val = config->entries[i].value;
            pthread_rwlock_unlock(&config->lock);
            return val;
        }
    }

    pthread_rwlock_unlock(&config->lock);
    return default_value;
}

int config_get_integer_t32(config_t32_t *config, const char *key, int default_value) {
    const char *s = config_get_string_t32(config, key, NULL);
    return s ? atoi(s) : default_value;
}

int config_get_boolean_t32(config_t32_t *config, const char *key, int default_value) {
    const char *s = config_get_string_t32(config, key, NULL);
    if (!s) return default_value;
    return (strcasecmp(s, "true") == 0 || strcasecmp(s, "yes") == 0 || atoi(s) != 0);
}

int config_set_value_t32(config_t32_t *config, const char *key, const char *value, config_type_t32_t type) {
    if (!config || !key || !value) return -1;

    pthread_rwlock_wrlock(&config->lock);

    for (int i = 0; i < config->entry_count; i++) {
        if (strcmp(config->entries[i].key, key) == 0) {
            strncpy(config->entries[i].value, value, CONFIG_MAX_VALUE - 1);
            config->entries[i].value[CONFIG_MAX_VALUE - 1] = '\0';
            pthread_rwlock_unlock(&config->lock);
            return 0;
        }
    }

    if (config->entry_count >= config->entry_capacity) {
        config->entry_capacity *= 2;
        config_entry_t32_t *grown = realloc(config->entries,
            sizeof(config_entry_t32_t) * config->entry_capacity);
        if (!grown) {
            pthread_rwlock_unlock(&config->lock);
            return -1;
        }
        config->entries = grown;
    }

    config_entry_t32_t *entry = &config->entries[config->entry_count];
    strncpy(entry->key,   key,   CONFIG_MAX_DIRECTIVE - 1);
    entry->key[CONFIG_MAX_DIRECTIVE - 1] = '\0';
    strncpy(entry->value, value, CONFIG_MAX_VALUE - 1);
    entry->value[CONFIG_MAX_VALUE - 1] = '\0';
    entry->type = type;

    config->entry_count++;
    pthread_rwlock_unlock(&config->lock);

    return 0;
}

int config_reload_t32(config_t32_t *config) {
    if (!config) return -1;

    pthread_rwlock_wrlock(&config->lock);
    config->entry_count = 0;
    pthread_rwlock_unlock(&config->lock);

    FILE *fp = fopen(config->config_file, "r");
    if (!fp) return -1;

    char line[CONFIG_MAX_VALUE];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;

        char *key   = line;
        char *value = eq + 1;
        *eq = '\0';

        while (*value && isspace((unsigned char)*value)) value++;
        int len = (int)strlen(value);
        while (len > 0 && isspace((unsigned char)value[len - 1])) value[--len] = '\0';

        config_set_value_t32(config, key, value, CONF_STRING);
    }

    fclose(fp);
    return 0;
}
