#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>
#include "xxhash.h"
#include "common.h"
#include "config.h"

static bool in_system_directory(const char *executable_path) {
    const char *system_dirs[] = { "/etc/", "/dev/", "/usr/", "/bin/", "/boot/", "/lib/", "/opt/", "/sbin/", "/sys/", "/proc/", "/var/" };
    size_t num_dirs = sizeof(system_dirs) / sizeof(system_dirs[0]);

    for (size_t i = 0; i < num_dirs; ++i) {
        if (strncmp(executable_path, system_dirs[i], strlen(system_dirs[i])) == 0) {
            return true;
        }
    }
    return false;
}

static int resolve_exe_and_hash(char *exe_out, char *hash_out) {
    ssize_t exe_len = readlink("/proc/self/exe", exe_out, PATH_MAX - 1);
    if (exe_len == -1) { return -1; }
    exe_out[exe_len] = '\0';

    XXH128_hash_t h = XXH3_128bits(exe_out, strlen(exe_out));
    snprintf(hash_out, 33, "%016llx%016llx", (unsigned long long)h.high64, (unsigned long long)h.low64);
    return 0;
}

static char* get_file_path_py(const char *executable_path) {
    // Match python interpreter by basename prefix (python, python3, python3.11, pythonw...)
    const char *basename = strrchr(executable_path, '/');
    basename = basename ? basename + 1 : executable_path;
    if (strncmp(basename, "python", 6) == 0) {
        FILE *cmdline = fopen("/proc/self/cmdline", "rb");
        if (!cmdline) { return NULL; }

        char *arg = NULL;
        size_t size = 0;
        char *file_path_py = NULL;

        // Check each argument from the command line
        while (getdelim(&arg, &size, 0, cmdline) != -1) {
            size_t arg_len = strlen(arg);
            if (arg_len >= 3 && strcmp(arg + arg_len - 3, ".py") == 0) {
                file_path_py = realpath(arg, NULL);
                break;
            }
        }

        free(arg);
        fclose(cmdline);
        return file_path_py;
    }
    return NULL;
}

__attribute__((constructor))
static void constructor_hook() {

    // Exit if not the main process
    if (parse_env_int("SLURM_PROCID", 0) != 0) { return; }

    char executable_path[PATH_MAX];
    char xxhash_string[33];
    if (resolve_exe_and_hash(executable_path, xxhash_string) != 0) { return; }

    // Collect for everything
    collect_info(executable_path, xxhash_string, "SELF", "INFO");
#if COLLECT_OBJECTS
    collect_objects(executable_path, xxhash_string, "SELF", "OBJECTS");
#endif

    // Collect for executables in user directory
    if (!in_system_directory(executable_path)) {
#if COLLECT_MODULES
        collect_modules(executable_path, xxhash_string, "SELF", "MODULES");
#endif
#if COLLECT_COMPILERS
        collect_compilers(executable_path, xxhash_string, "SELF", "COMPILERS");
#endif
#if COLLECT_FILE_H
        collect_ssdeep_file(executable_path, xxhash_string, "SELF", "FILE_H");
#endif
#if COLLECT_STRINGS_H
        collect_ssdeep_strings(executable_path, xxhash_string, "SELF", "STRINGS_H");
#endif
#if COLLECT_SYMBOLS_H
        collect_ssdeep_symbols(executable_path, xxhash_string, "SELF", "SYMBOLS_H");
#endif
    }

    // Collect for python
    char *file_path_py = get_file_path_py(executable_path);
    if (file_path_py) {
        collect_info(file_path_py, xxhash_string, "SCRIPT", "INFO");
#if COLLECT_FILE_H
        collect_ssdeep_file(file_path_py, xxhash_string, "SCRIPT", "FILE_H");
#endif
        free(file_path_py);
    }
}

__attribute__((destructor))
static void destructor_hook() {

    // Exit if not the main process
    if (parse_env_int("SLURM_PROCID", 0) != 0) { return; }

#if COLLECT_MAPS
    char executable_path[PATH_MAX];
    char xxhash_string[33];
    if (resolve_exe_and_hash(executable_path, xxhash_string) != 0) { return; }

    // Collect for executables in user directory
    if (!in_system_directory(executable_path)) {
        collect_maps(executable_path, xxhash_string, "SELF", "MAPS");
        return; // avoid double collection for python
    }

    // Collect for python interpreters in system directory
    const char *basename = strrchr(executable_path, '/');
    basename = basename ? basename + 1 : executable_path;
    if (strncmp(basename, "python", 6) == 0) {
        collect_maps(executable_path, xxhash_string, "SELF", "MAPS");
    }
#endif
}