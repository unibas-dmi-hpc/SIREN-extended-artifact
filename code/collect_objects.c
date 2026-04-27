#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <link.h>
#include <dlfcn.h>
#include "common.h"

struct objects_ctx {
    char *buf;
    int offset;
};

// Callback function to process each shared object
static int process_objects_dl(struct dl_phdr_info *info, size_t size, void *data) {
    (void)size;

    struct objects_ctx *ctx = (struct objects_ctx *)data;

    // Skip if the object name is null or empty
    if (info->dlpi_name == NULL || info->dlpi_name[0] == '\0') { return 0; }

    const char *object_path = info->dlpi_name;
    size_t object_path_len = strlen(object_path);

    // Check if element fits into the buffer
    if (ctx->offset + object_path_len + 2 >= CONTENT_BUFFER) { return 1; }

    ctx->offset += snprintf(ctx->buf + ctx->offset, CONTENT_BUFFER - ctx->offset, "%s;", object_path);

    return 0;
}

void collect_objects(const char *target_file_path, const char *xxhash_string, const char *content_layer, const char *content_type) {
    (void)target_file_path;

    // Allocate message buffer
    char *message_content = malloc(CONTENT_BUFFER);
    if (!message_content) { return; }
    message_content[0] = '\0';

    // Collect shared objects
    struct objects_ctx ctx = { .buf = message_content, .offset = 0 };
    dl_iterate_phdr(process_objects_dl, &ctx);

    // Check if we found information
    if (ctx.offset == 0) {
        snprintf(message_content, CONTENT_BUFFER, "N/A;");
        prepare_single_message(message_content, xxhash_string, content_layer, content_type);
    } else {
        prepare_hash_chunk_message(message_content, xxhash_string, content_layer, content_type);
    }
    
    // Free the dynamically allocated buffer
    free(message_content);
}