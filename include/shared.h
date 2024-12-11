#include <arpa/inet.h>
#include <time.h>
#include <glib.h>

#ifndef SHARED_H
#define SHARED_H

#define BUFFER_SIZE 32768

unsigned short calculate_checksum(void *b, int len);

typedef struct {
    int sockfd;
    struct sockaddr_in client_addr;
    socklen_t addr_len;
    GHashTable* thread_table;
} client_data_t;

#endif