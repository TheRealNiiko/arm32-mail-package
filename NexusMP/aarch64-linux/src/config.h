#ifndef CONFIG_H
#define CONFIG_H

#include "pthread.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

typedef struct {
    char *key;
    char *value;
} config_entry_t;

typedef struct {
    config_entry_t *entries;
    unsigned int entry_count;
    unsigned int entry_capacity;
} config_t;

config_t *config_create(void);
void config_free(config_t *cfg);

int config_load_file(config_t *cfg, const char *filename);
int config_set_value(config_t *cfg, const char *key, const char *value);

const char *config_get_string(const config_t *cfg, const char *key, const char *default_value);
int config_get_int(const config_t *cfg, const char *key, int default_value);
int config_get_bool(const config_t *cfg, const char *key, int default_value);

char *config_get_directory(const char *type);

#endif
