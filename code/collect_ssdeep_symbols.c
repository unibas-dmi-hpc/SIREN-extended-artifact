#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <libelf.h>
#include <gelf.h>
#include <fuzzy.h>
#include "common.h"

static void get_symbols(const char *target_file_path, char *message_content) {
    int message_offset = 0;

    // Initialize ELF library
    if (elf_version(EV_CURRENT) == EV_NONE) { return; }

    // Open file
    int fd = open(target_file_path, O_RDONLY);
    if (fd < 0) { return; }

    // Begin reading ELF file
    Elf *elf = elf_begin(fd, ELF_C_READ, NULL);
    if (!elf) { close(fd); return; }

    Elf_Scn *scn = NULL;
    GElf_Shdr shdr;

    // Iterate over all sections in the ELF file
    while ((scn = elf_nextscn(elf, scn)) != NULL) {
        if (gelf_getshdr(scn, &shdr) != &shdr) { continue; }

        // Process symbol tables
        if (shdr.sh_type == SHT_SYMTAB || shdr.sh_type == SHT_DYNSYM) {
            Elf_Data *data = elf_getdata(scn, NULL);
            
            if (!data) { continue; }
            if (shdr.sh_entsize == 0) { continue; }
            
            int count = shdr.sh_size / shdr.sh_entsize;

            // Iterate over all symbols in the symbol table
            for (int i = 0; i < count; ++i) {
                GElf_Sym sym;
                if (gelf_getsym(data, i, &sym) != &sym) { continue; }

                // Process global symbols
                if (GELF_ST_BIND(sym.st_info) == STB_GLOBAL) {
                    const char *symbol_name = elf_strptr(elf, shdr.sh_link, sym.st_name);
                    if (symbol_name) {

                        if (message_offset + strlen(symbol_name) + 2 > CONTENT_BUFFER) { elf_end(elf); close(fd); return; }
                        message_offset += snprintf(message_content + message_offset, CONTENT_BUFFER - message_offset, "%s\n", symbol_name);
                    }
                }
            }
        }
    }

    elf_end(elf);
    close(fd);
}

void collect_ssdeep_symbols(const char *target_file_path, const char *xxhash_string, const char *content_layer, const char *content_type) {
    // Allocate message buffer
    char *message_content = malloc(CONTENT_BUFFER);
    if (!message_content) { return; }
    message_content[0] = '\0';

    // Collect symbol information
    get_symbols(target_file_path, message_content);

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