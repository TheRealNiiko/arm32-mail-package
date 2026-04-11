#include "search_amd64.h"
#include <dirent.h>
#include <stdio.h>

search_result_amd64_t *search_execute_amd64(const char *maildir_path, search_filter_amd64_t *filters, int filter_count) {
    if (!maildir_path) return NULL;

    search_result_amd64_t *result = malloc(sizeof(search_result_amd64_t));
    if (!result) return NULL;

    result->uids = malloc(sizeof(uint32_t) * 256);
    if (!result->uids) {
        free(result);
        return NULL;
    }

    result->uid_count = 0;
    result->uid_capacity = 256;

    return result;
}

void search_free_result_amd64(search_result_amd64_t *result) {
    if (!result) return;
    if (result->uids) free(result->uids);
    free(result);
}

int search_add_uid_amd64(search_result_amd64_t *result, uint32_t uid) {
    if (!result) return -1;

    if (result->uid_count >= result->uid_capacity) {
        result->uid_capacity *= 2;
        uint32_t *new_uids = realloc(result->uids, sizeof(uint32_t) * result->uid_capacity);
        if (!new_uids) return -1;
        result->uids = new_uids;
    }

    result->uids[result->uid_count++] = uid;
    return 0;
}

char *search_format_response_amd64(search_result_amd64_t *result) {
    if (!result) return strdup("* SEARCH\r\n");

    size_t buf_size = result->uid_count * 20 + 50;
    char *response = malloc(buf_size);
    if (!response) return NULL;

    strcpy(response, "* SEARCH");
    for (int i = 0; i < result->uid_count; i++) {
        char uid_str[20];
        snprintf(uid_str, sizeof(uid_str), " %u", result->uids[i]);
        strcat(response, uid_str);
    }
    strcat(response, "\r\n");

    return response;
}

search_filter_amd64_t *search_parse_criteria_amd64(const char *criteria_str, int *filter_count) {
    if (!criteria_str || !filter_count) return NULL;

    search_filter_amd64_t *filters = malloc(sizeof(search_filter_amd64_t) * 16);
    if (!filters) return NULL;

    *filter_count = 0;

    return filters;
}

void search_free_filters_amd64(search_filter_amd64_t *filters, int count) {
    if (!filters) return;
    free(filters);
}
