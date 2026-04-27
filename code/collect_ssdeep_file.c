#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fuzzy.h>
#include "common.h"

void collect_ssdeep_file(const char *target_file_path, const char *xxhash_string, const char *content_layer, const char *content_type) {
    // Allocate message buffer
    char *message_content = malloc(CONTENT_BUFFER);
    if (!message_content) { return; }
    message_content[0] = '\0';
    
    char file_hash[FUZZY_MAX_RESULT];

    // Open file
    FILE *file = fopen(target_file_path, "rb");
    if (file) {
        // Generate fuzzy hash
        int hash_status = fuzzy_hash_file(file, file_hash);
        if (hash_status == 0) {
            snprintf(message_content, CONTENT_BUFFER, "%s;", file_hash);
        }
        fclose(file);
    }

    // Check if we found information
    if (strlen(message_content) == 0) {
        snprintf(message_content, CONTENT_BUFFER, "N/A;");
    }
    prepare_single_message(message_content, xxhash_string, content_layer, content_type);

    // Free the dynamically allocated buffer
    free(message_content);
}