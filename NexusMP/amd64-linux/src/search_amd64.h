#ifndef SEARCH_AMD64_H
#define SEARCH_AMD64_H

#include "pthread.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    uint32_t *uids;
    int uid_count;
    int uid_capacity;
} search_result_amd64_t;

typedef enum {
    SEARCH_ALL,
    SEARCH_TEXT,
    SEARCH_SUBJECT,
    SEARCH_FROM,
    SEARCH_TO,
    SEARCH_BEFORE,
    SEARCH_SINCE,
    SEARCH_FLAGGED,
    SEARCH_UNFLAGGED,
    SEARCH_DELETED,
    SEARCH_UNDELETED
} search_criteria_type_amd64_t;

typedef struct {
    search_criteria_type_amd64_t type;
    char value[512];
    time_t date_value;
} search_filter_amd64_t;

search_result_amd64_t *search_execute_amd64(const char *maildir_path, search_filter_amd64_t *filters, int filter_count);
void search_free_result_amd64(search_result_amd64_t *result);

int search_add_uid_amd64(search_result_amd64_t *result, uint32_t uid);
char *search_format_response_amd64(search_result_amd64_t *result);

search_filter_amd64_t *search_parse_criteria_amd64(const char *criteria_str, int *filter_count);
void search_free_filters_amd64(search_filter_amd64_t *filters, int count);

#endif
