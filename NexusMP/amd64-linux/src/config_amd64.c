#include "config_amd64.h"
#include <ctype.h>
#include <stdio.h>

config_t_amd64 *config_load_amd64(const char *config_file) {
    if (!config_file) return NULL;

    config_t_amd64 *config = malloc(sizeof(config_t_amd64));
    if (!config) return NULL;

    strncpy(config->config_file, config_file, 511);
    config->config_file[511] = '\0';

    config->entries = malloc(sizeof(config_entry_amd64_t) * 64);
    if (!config->entries) {
        free(config);
        return NULL;
    }

    config->entry_count = 0;
    config->entry_capacity = 64;
    pthread_rwlock_init(&config->lock, NULL);

    FILE *fp = fopen(config_file, "r");
    if (!fp) {
        return config; /* Return empty config */
    }

    char line[2048];
    while (fgets(line, sizeof(line), fp)) {
        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\n') continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0';
        char *value = eq + 1;

        /* Remove whitespace */
        while (isspace(*line)) line++;
        while (isspace(*(eq - 1))) *(eq - 1) = '\0';
        while (isspace(*value)) value++;
        while (isspace(*(value + strlen(value) - 1))) *(value + strlen(value) - 1) = '\0';

        config_set_value_amd64(config, line, value, CONF_STRING);
    }

    fclose(fp);
    return config;
}

void config_free_amd64(config_t_amd64 *config) {
    if (!config) return;

    pthread_rwlock_destroy(&config->lock);
    if (config->entries) {
        free(config->entries);
    }
    free(config);
}

const char *config_get_string_amd64(config_t_amd64 *config, const char *key, const char *default_value) {
    if (!config || !key) return default_value;

    pthread_rwlock_rdlock(&config->lock);

    for (int i = 0; i < config->entry_count; i++) {
        if (strcmp(config->entries[i].key, key) == 0) {
            const char *value = config->entries[i].value;
            pthread_rwlock_unlock(&config->lock);
            return value;
        }
    }

    pthread_rwlock_unlock(&config->lock);
    return default_value;
}

int config_get_integer_amd64(config_t_amd64 *config, const char *key, int default_value) {
    const char *str_value = config_get_string_amd64(config, key, NULL);
    if (!str_value) return default_value;

    return atoi(str_value);
}

int config_get_boolean_amd64(config_t_amd64 *config, const char *key, int default_value) {
    const char *str_value = config_get_string_amd64(config, key, NULL);
    if (!str_value) return default_value;

    return (strcasecmp(str_value, "true") == 0 || strcasecmp(str_value, "yes") == 0 || atoi(str_value) != 0);
}

int config_set_value_amd64(config_t_amd64 *config, const char *key, const char *value, config_type_amd64_t type) {
    if (!config || !key || !value) return -1;

    pthread_rwlock_wrlock(&config->lock);

    /* Check if key already exists */
    for (int i = 0; i < config->entry_count; i++) {
        if (strcmp(config->entries[i].key, key) == 0) {
            strncpy(config->entries[i].value, value, CONFIG_MAX_VALUE - 1);
            config->entries[i].value[CONFIG_MAX_VALUE - 1] = '\0';
            pthread_rwlock_unlock(&config->lock);
            return 0;
        }
    }

    /* Add new entry */
    if (config->entry_count >= config->entry_capacity) {
        config->entry_capacity *= 2;
        config_entry_amd64_t *new_entries = realloc(config->entries, sizeof(config_entry_amd64_t) * config->entry_capacity);
        if (!new_entries) {
            pthread_rwlock_unlock(&config->lock);
            return -1;
        }
        config->entries = new_entries;
    }

    config_entry_amd64_t *entry = &config->entries[config->entry_count];
    strncpy(entry->key, key, CONFIG_MAX_DIRECTIVE - 1);
    entry->key[CONFIG_MAX_DIRECTIVE - 1] = '\0';
    strncpy(entry->value, value, CONFIG_MAX_VALUE - 1);
    entry->value[CONFIG_MAX_VALUE - 1] = '\0';
    entry->type = type;

    config->entry_count++;
    pthread_rwlock_unlock(&config->lock);

    return 0;
}

int config_reload_amd64(config_t_amd64 *config) {
    if (!config) return -1;

    /* Clear old entries */
    pthread_rwlock_wrlock(&config->lock);
    config->entry_count = 0;
    pthread_rwlock_unlock(&config->lock);

    /* Reload from file */
    FILE *fp = fopen(config->config_file, "r");
    if (!fp) return -1;

    char line[2048];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n') continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;

        char *key = line;
        char *value = eq + 1;
        *eq = '\0';

        while (isspace(*value)) value++;
        int len = strlen(value);
        while (len > 0 && isspace(value[len - 1])) value[--len] = '\0';

        config_set_value_amd64(config, key, value, CONF_STRING);
    }

    fclose(fp);
    return 0;
}
