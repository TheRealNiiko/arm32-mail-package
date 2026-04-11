#include "search_termux32.h"
#include <dirent.h>
#include <stdio.h>

search_result_t32_t *search_execute_t32(const char *maildir_path,
                                         search_filter_t32_t *filters,
                                         int filter_count) {
    if (!maildir_path) return NULL;
    (void)filters; (void)filter_count;

    search_result_t32_t *result = malloc(sizeof(search_result_t32_t));
    if (!result) return NULL;

    /* ARM32: start with 64 UID slots (was 256) */
    result->uids = malloc(sizeof(uint32_t) * 64);
    if (!result->uids) {
        free(result);
        return NULL;
    }

    result->uid_count    = 0;
    result->uid_capacity = 64;

    return result;
}

void search_free_result_t32(search_result_t32_t *result) {
    if (!result) return;
    free(result->uids);
    free(result);
}

int search_add_uid_t32(search_result_t32_t *result, uint32_t uid) {
    if (!result) return -1;

    if (result->uid_count >= result->uid_capacity) {
        result->uid_capacity *= 2;
        uint32_t *grown = realloc(result->uids, sizeof(uint32_t) * result->uid_capacity);
        if (!grown) return -1;
        result->uids = grown;
    }

    result->uids[result->uid_count++] = uid;
    return 0;
}

char *search_format_response_t32(search_result_t32_t *result) {
    if (!result) return strdup("* SEARCH\r\n");

    /* ARM32: 12 chars per UID + header */
    size_t buf_size = (size_t)result->uid_count * 12 + 32;
    char *response = malloc(buf_size);
    if (!response) return NULL;

    strcpy(response, "* SEARCH");
    for (int i = 0; i < result->uid_count; i++) {
        char uid_str[12];
        snprintf(uid_str, sizeof(uid_str), " %u", result->uids[i]);
        strcat(response, uid_str);
    }
    strcat(response, "\r\n");

    return response;
}

search_filter_t32_t *search_parse_criteria_t32(const char *criteria_str, int *filter_count) {
    if (!criteria_str || !filter_count) return NULL;

    /* ARM32: start with 8 filter slots (was 16) */
    search_filter_t32_t *filters = malloc(sizeof(search_filter_t32_t) * 8);
    if (!filters) return NULL;

    *filter_count = 0;
    return filters;
}

void search_free_filters_t32(search_filter_t32_t *filters, int count) {
    (void)count;
    free(filters);
}
