#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <libelf.h>
#include <gelf.h>
#include "common.h"

static void get_compilers(const char *target_file_path, char *message_content) {
    // Initialize ELF library
    if (elf_version(EV_CURRENT) == EV_NONE) { return; }

    // Open file
    int fd = open(target_file_path, O_RDONLY);
    if (fd < 0) { return; }

    // Begin reading ELF file
    Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
    if (!elf) { close(fd); return; }

    // Get ELF header
    GElf_Ehdr ehdr;
    if (!gelf_getehdr(elf, &ehdr)) { elf_end(elf); close(fd); return; }

    // Find .comment section
    int message_offset = 0;
    Elf_Scn *scn = NULL;
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        GElf_Shdr shdr;
        if (!gelf_getshdr(scn, &shdr)) { continue; }

        // Process data if we found the .comment section
        char *name = elf_strptr(elf, ehdr.e_shstrndx, shdr.sh_name);
        if (name && strcmp(name, ".comment") == 0) {
            Elf_Data *data = NULL;
            while ((data = elf_getdata(scn, data)) != NULL) {
                if (data->d_buf && data->d_size > 0) {
                    char *comment = (char *)data->d_buf;
                    char *end = comment + data->d_size;

                    // Search compiler information
                    while (comment < end) {
                        size_t remaining = (size_t)(end - comment);
                        size_t len = strnlen(comment, remaining);
                        if (len == 0 || len == remaining) { break; }

                        // Check if element fits into the buffer
                        if (message_offset + len + 2 >= CONTENT_BUFFER) { elf_end(elf); close(fd); return; }
                        message_offset += snprintf(message_content + message_offset, CONTENT_BUFFER - message_offset, "%s;", comment);

                        comment += len + 1;
                    }
                }
            }
        }
    }

    elf_end(elf);
    close(fd);
}

void collect_compilers(const char *target_file_path, const char *xxhash_string, const char *content_layer, const char *content_type) {
    // Allocate message buffer
    char *message_content = malloc(CONTENT_BUFFER);
    if (!message_content) { return; }
    message_content[0] = '\0';
    
    // Collect compiler information
    get_compilers(target_file_path, message_content);

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