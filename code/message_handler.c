#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <limits.h>
#include <fuzzy.h>
#include "common.h"
#include "config.h"

time_t get_init_time(void) {
    static time_t cached = 0;
    if (cached == 0) { cached = time(NULL); }
    return cached;
}

const char* get_host_name(void) {
    static char buf[HOST_NAME_MAX + 1] = {0};
    if (buf[0] == 0) {
        if (gethostname(buf, sizeof(buf) - 1) != 0) {
            const char *env = getenv("HOSTNAME");
            strncpy(buf, env ? env : "-1", sizeof(buf) - 1);
        }
    }
    return buf;
}

void send_udp_message(const char *message_payload) {
    int sockfd;
    struct sockaddr_in server_addr;

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { return; }

    // Add server information
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);
    if (inet_pton(AF_INET, UDP_IP, &server_addr.sin_addr) <= 0) { close(sockfd); return; }

    // Send UDP packet
    sendto(sockfd, message_payload, strlen(message_payload), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    close(sockfd);
}

void prepare_single_message(char *message_content, const char *xxhash_string, const char *content_layer, const char *content_type) {
    char message_payload[MESSAGE_BUFFER];
    snprintf(message_payload, MESSAGE_BUFFER, "JOBIDRAW=%d|STEPID=%d|PID=%d|HASH=%s|HOST=%s|TIME=%ld|LAYER=%s|TYPE=%s|CONTENT=%s", JOB_ID, STEP_ID, PROCESS_ID, xxhash_string, HOST_NAME, INIT_TIME, content_layer, content_type, message_content);

    send_udp_message(message_payload);
}

void prepare_hash_chunk_message(char *message_content, const char *xxhash_string, const char *content_layer, const char *content_type) {
    char message_payload[MESSAGE_BUFFER];

    // Compute SSDeep hash
    char content_hash[FUZZY_MAX_RESULT] = {0};
#if COLLECT_CHUNK_H
    int hash_status = fuzzy_hash_buf((unsigned char*)message_content, strlen(message_content), content_hash);
    if (hash_status != 0) { snprintf(content_hash, FUZZY_MAX_RESULT, "fuzzy_hash_buf()failed;"); }
#endif

    // Tokenize message_content
    char *saveptr;
    char *token = strtok_r(message_content, ";", &saveptr);
    int message_number = 0;

    // Send message_content in chunks
    while (token != NULL && message_number < MAX_MESSAGES) {
        message_number++;

        // Header for the current chunk
        int header_size = snprintf(message_payload, MESSAGE_BUFFER, "JOBIDRAW=%d|STEPID=%d|PID=%d|HASH=%s|HOST=%s|TIME=%ld|LAYER=%s|TYPE=%s_%d|CONTENT=", JOB_ID, STEP_ID, PROCESS_ID, xxhash_string, HOST_NAME, INIT_TIME, content_layer, content_type, message_number);

        // Add SSDeep hash
        if (message_number == 1) {
            header_size += snprintf(message_payload + header_size, MESSAGE_BUFFER - header_size, "%s;", content_hash);
        }

        int remaining_space = MESSAGE_BUFFER - header_size - 1;
        int buffer_index = header_size;

        // Add tokens to current chunk
        while (token != NULL) {
            size_t token_len = strlen(token);
            int required_space = token_len + 1;

            // Break if we reached the buffer limit
            if (required_space > remaining_space) {
                // Nothing fit in this chunk, skip the oversize token to avoid empty-datagram spam
                if (buffer_index == header_size) { token = strtok_r(NULL, ";", &saveptr); }
                break;
            }

            // Append token and separator
            snprintf(message_payload + buffer_index, remaining_space + 1, "%s;", token);
            buffer_index += required_space;
            remaining_space -= required_space;

            // Move to the next token
            token = strtok_r(NULL, ";", &saveptr);
        }

        send_udp_message(message_payload);
    }

    // If tokens remain after MAX_MESSAGES chunks, emit a truncation message
    if (token != NULL) {
        int remaining_token_count = 0;
        while (token != NULL) {
            remaining_token_count++;
            token = strtok_r(NULL, ";", &saveptr);
        }
        snprintf(message_payload, MESSAGE_BUFFER, "JOBIDRAW=%d|STEPID=%d|PID=%d|HASH=%s|HOST=%s|TIME=%ld|LAYER=%s|TYPE=%s_TRUNCATED|CONTENT=remaining_token_count=%d;", JOB_ID, STEP_ID, PROCESS_ID, xxhash_string, HOST_NAME, INIT_TIME, content_layer, content_type, remaining_token_count);
        send_udp_message(message_payload);
    }
}