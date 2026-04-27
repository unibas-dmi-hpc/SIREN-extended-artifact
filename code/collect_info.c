#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "common.h"

void collect_info(const char *target_file_path, const char *xxhash_string, const char *content_layer, const char *content_type) {
    // Allocate message buffer
    char *message_content = malloc(CONTENT_BUFFER);
    if (!message_content) { return; }
    message_content[0] = '\0';

    // Add identifiers
    int message_offset = snprintf(message_content, CONTENT_BUFFER, "EXE:%s; PPID:%d; UID:%u; GID:%u;", target_file_path, getppid(), getuid(), getgid());

    // Add stat() information
    struct stat file_stat;
    if (stat(target_file_path, &file_stat) == 0) {
        snprintf(message_content + message_offset, CONTENT_BUFFER - message_offset, " InodeNumber:%ld; FileSize:%ld; Permissions:0%o; FileOwnerUID:%u; FileOwnerGID:%u; Access:%ld; Modify:%ld; Change:%ld;", (long)file_stat.st_ino, (long)file_stat.st_size, file_stat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO), file_stat.st_uid, file_stat.st_gid, (long)file_stat.st_atime, (long)file_stat.st_mtime, (long)file_stat.st_ctime);
    } else {
        snprintf(message_content + message_offset, CONTENT_BUFFER - message_offset, " StatError:%d;", errno);
    }

    prepare_single_message(message_content, xxhash_string, content_layer, content_type);
    
    // Free the dynamically allocated buffer
    free(message_content);
}