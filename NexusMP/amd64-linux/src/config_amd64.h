#ifndef CONFIG_AMD64_H
#define CONFIG_AMD64_H

#include "pthread.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define CONFIG_MAX_DIRECTIVE 256
#define CONFIG_MAX_VALUE 2048

typedef enum {
    CONF_STRING,
    CONF_INTEGER,
    CONF_BOOLEAN,
    CONF_PATH
} config_type_amd64_t;

typedef struct {
    char key[CONFIG_MAX_DIRECTIVE];
    char value[CONFIG_MAX_VALUE];
    config_type_amd64_t type;
} config_entry_amd64_t;

typedef struct {
    config_entry_amd64_t *entries;
    int entry_count;
    int entry_capacity;
    char config_file[512];
    pthread_rwlock_t lock;
} config_t_amd64;

config_t_amd64 *config_load_amd64(const char *config_file);
void config_free_amd64(config_t_amd64 *config);

const char *config_get_string_amd64(config_t_amd64 *config, const char *key, const char *default_value);
int config_get_integer_amd64(config_t_amd64 *config, const char *key, int default_value);
int config_get_boolean_amd64(config_t_amd64 *config, const char *key, int default_value);

int config_set_value_amd64(config_t_amd64 *config, const char *key, const char *value, config_type_amd64_t type);
int config_reload_amd64(config_t_amd64 *config);

#endif
