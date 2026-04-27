#define _GNU_SOURCE
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#define MAX_MESSAGES 5
#define MESSAGE_BUFFER (1024 * 8)
#define CONTENT_BUFFER (1024 * 1024)

#define UDP_IP "127.0.0.1"
#define UDP_PORT 4444

#define JOB_ID get_job_id()
#define STEP_ID get_step_id()
#define PROCESS_ID get_process_id()
#define HOST_NAME get_host_name()
#define INIT_TIME get_init_time()

static inline int parse_env_int(const char *name, int fallback) {
    const char *s = getenv(name);
    if (!s || !*s) { return fallback; }
    char *end;
    long v = strtol(s, &end, 10);
    if (*end != '\0') { return fallback; }
    return (int)v;
}
static inline int get_job_id()  { return parse_env_int("SLURM_JOB_ID",  -1); }
static inline int get_step_id() { return parse_env_int("SLURM_STEP_ID", -1); }
static inline int get_process_id() { return getpid(); }
const char* get_host_name(void);
time_t get_init_time(void);

void prepare_single_message(char *message_content, const char *xxhash_string, const char *content_layer, const char *content_type);
void prepare_hash_chunk_message(char *message_content, const char *xxhash_string, const char *content_layer, const char *content_type);

void collect_info(const char *target_file_path, const char *xxhash_string, const char *content_layer, const char *content_type);
void collect_objects(const char *target_file_path, const char *xxhash_string, const char *content_layer, const char *content_type);
void collect_modules(const char *target_file_path, const char *xxhash_string, const char *content_layer, const char *content_type);
void collect_compilers(const char *target_file_path, const char *xxhash_string, const char *content_layer, const char *content_type);
void collect_maps(const char *target_file_path, const char *xxhash_string, const char *content_layer, const char *content_type);

void collect_ssdeep_file(const char *target_file_path, const char *xxhash_string, const char *content_layer, const char *content_type);
void collect_ssdeep_strings(const char *target_file_path, const char *xxhash_string, const char *content_layer, const char *content_type);
void collect_ssdeep_symbols(const char *target_file_path, const char *xxhash_string, const char *content_layer, const char *content_type);

#endif