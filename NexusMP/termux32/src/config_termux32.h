#ifndef CONFIG_TERMUX32_H
#define CONFIG_TERMUX32_H

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define CONFIG_MAX_DIRECTIVE  128    /* ARM32: tightened from 256 */
#define CONFIG_MAX_VALUE      1024   /* ARM32: halved from 2048 */

typedef enum {
    CONF_STRING,
    CONF_INTEGER,
    CONF_BOOLEAN,
    CONF_PATH
} config_type_t32_t;

typedef struct {
    char key[CONFIG_MAX_DIRECTIVE];
    char value[CONFIG_MAX_VALUE];
    config_type_t32_t type;
} config_entry_t32_t;

typedef struct {
    config_entry_t32_t *entries;
    int                 entry_count;
    int                 entry_capacity;
    char                config_file[256];   /* ARM32: halved from 512 */
    pthread_rwlock_t    lock;
} config_t32_t;

config_t32_t *config_load_t32(const char *config_file);
void config_free_t32(config_t32_t *config);

const char *config_get_string_t32(config_t32_t *config, const char *key, const char *default_value);
int         config_get_integer_t32(config_t32_t *config, const char *key, int default_value);
int         config_get_boolean_t32(config_t32_t *config, const char *key, int default_value);

int config_set_value_t32(config_t32_t *config, const char *key, const char *value, config_type_t32_t type);
int config_reload_t32(config_t32_t *config);

#endif
