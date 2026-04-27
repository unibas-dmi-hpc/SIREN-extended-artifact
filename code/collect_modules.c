#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

void collect_modules(const char *target_file_path, const char *xxhash_string, const char *content_layer, const char *content_type) {
    (void)target_file_path;
    
    // Allocate message buffer
    char *message_content = malloc(CONTENT_BUFFER);
    if (!message_content) { return; }
    message_content[0] = '\0';

    // Collect loaded modules and replace : with ;
    const char *loaded_modules = getenv("LOADEDMODULES");
    snprintf(message_content, CONTENT_BUFFER, "%s", loaded_modules ? loaded_modules : "");
    for (char *p = message_content; *p; p++) { if (*p == ':') { *p = ';'; } }

    // Check if we found information
    if (strlen(message_content) == 0) { 
        snprintf(message_content, CONTENT_BUFFER, "N/A;");
        prepare_single_message(message_content, xxhash_string, content_layer, content_type);
    } else {
        prepare_hash_chunk_message(message_content, xxhash_string, content_layer, content_type);
    }
    
    // Free the dynamically allocated buffer
    free(message_content);
}