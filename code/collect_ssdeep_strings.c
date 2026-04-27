#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fuzzy.h>
#include "common.h"

#define MIN_STRING_LENGTH 4
#define MAX_STRING_LENGTH 256

static void get_strings(const char *target_file_path, char *message_content) {
    FILE *file_pointer = fopen(target_file_path, "rb");
    if (!file_pointer) { return; }

    int message_offset = 0;
    char string_buffer[MAX_STRING_LENGTH];
    int buffer_index = 0;

    // Iterate over characters
    int current_char;    
    while ((current_char = fgetc(file_pointer)) != EOF) {
        if (isprint(current_char)) {
            string_buffer[buffer_index++] = current_char;
            if (buffer_index + 1 >= MAX_STRING_LENGTH) {
                string_buffer[buffer_index] = '\0';

                if (message_offset + buffer_index + 2 > CONTENT_BUFFER) { fclose(file_pointer); return; }
                message_offset += snprintf(message_content + message_offset, CONTENT_BUFFER - message_offset, "%s\n", string_buffer);

                buffer_index = 0;
            }
        } else { // non-printable character
            if (buffer_index >= MIN_STRING_LENGTH) {
                string_buffer[buffer_index] = '\0';

                if (message_offset + buffer_index + 2 > CONTENT_BUFFER) { fclose(file_pointer); return; }
                message_offset += snprintf(message_content + message_offset, CONTENT_BUFFER - message_offset, "%s\n", string_buffer);
            }
            buffer_index = 0;
        }
    }

    // Check remaining string buffer
    if (buffer_index >= MIN_STRING_LENGTH) {
        string_buffer[buffer_index] = '\0';

        if (message_offset + buffer_index + 2 > CONTENT_BUFFER) { fclose(file_pointer); return; }
        message_offset += snprintf(message_content + message_offset, CONTENT_BUFFER - message_offset, "%s\n", string_buffer);
    }

    fclose(file_pointer);
}

void collect_ssdeep_strings(const char *target_file_path, const char *xxhash_string, const char *content_layer, const char *content_type) {
    // Allocate message buffer
    char *message_content = malloc(CONTENT_BUFFER);
    if (!message_content) { return; }
    message_content[0] = '\0';

    // Collect printable characters
    get_strings(target_file_path, message_content);

    // Compute SSDeep hash
    char content_hash[FUZZY_MAX_RESULT] = {0};
    int hash_status = fuzzy_hash_buf((unsigned char*)message_content, strlen(message_content), content_hash);
    if (hash_status != 0) { snprintf(message_content, CONTENT_BUFFER, "N/A;"); }
    else { snprintf(message_content, CONTENT_BUFFER, "%s;", content_hash); }

    // Send message
    prepare_single_message(message_content, xxhash_string, content_layer, content_type);

    // Free the dynamically allocated buffer
    free(message_content);
}