#ifndef SEARCH_TERMUX32_H
#define SEARCH_TERMUX32_H

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    uint32_t *uids;
    int       uid_count;
    int       uid_capacity;
} search_result_t32_t;

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
} search_criteria_type_t32_t;

typedef struct {
    search_criteria_type_t32_t type;
    char   value[256];          /* ARM32: halved from 512 */
    time_t date_value;
} search_filter_t32_t;

search_result_t32_t *search_execute_t32(const char *maildir_path,
                                         search_filter_t32_t *filters,
                                         int filter_count);
void search_free_result_t32(search_result_t32_t *result);

int   search_add_uid_t32(search_result_t32_t *result, uint32_t uid);
char *search_format_response_t32(search_result_t32_t *result);

search_filter_t32_t *search_parse_criteria_t32(const char *criteria_str, int *filter_count);
void search_free_filters_t32(search_filter_t32_t *filters, int count);

#endif
