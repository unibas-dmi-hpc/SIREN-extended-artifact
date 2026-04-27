#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include "common.h"

static void get_maps(char *message_content) {
    // Open the maps file
    char maps_path[PATH_MAX];
    snprintf(maps_path, PATH_MAX, "/proc/%d/maps", PROCESS_ID);
    FILE *maps_file = fopen(maps_path, "r");
    if (!maps_file) { return; }

    // Collect memory mapped regions
    int message_offset = 0;
    char line[PATH_MAX + 128];
    while (fgets(line, sizeof(line), maps_file)) {
        // Find the region path in line
        char *path = strchr(line, '/');
        if (!path) { continue; }

        // Remove newline character at the end of the path
        size_t len = strlen(path);
        if (len > 0 && path[len - 1] == '\n') { path[len - 1] = '\0'; }

        // Check if entry is already in message_content (boundary-safe dedup)
        char temp_path[PATH_MAX + 2];
        snprintf(temp_path, sizeof(temp_path), "%s;", path);
        int is_duplicate = 0;
        for (char *found = strstr(message_content, temp_path); found != NULL; found = strstr(found + 1, temp_path)) {
            if (found == message_content || *(found - 1) == ';') { is_duplicate = 1; break; }
        }
        if (!is_duplicate) {
            // Check if entry + ";" fits message_content
            if (message_offset + len + 2 >= CONTENT_BUFFER) { fclose(maps_file); return; }
            message_offset += snprintf(message_content + message_offset, CONTENT_BUFFER - message_offset, "%s;", path);
        }
    }

    fclose(maps_file);
}

void collect_maps(const char *target_file_path, const char *xxhash_string, const char *content_layer, const char *content_type) {
    (void)target_file_path;
    
    // Allocate message buffer
    char *message_content = malloc(CONTENT_BUFFER);
    if (!message_content) { return; }
    message_content[0] = '\0';
    
    // Collect memory-mapped regions
    get_maps(message_content);

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