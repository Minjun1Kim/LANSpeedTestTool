#include <arpa/inet.h>

typedef struct {
    int sockfd;
    struct sockaddr_in client_addr;
    socklen_t addr_len;
    char test[10];
} client_data_t;

struct packet {
    struct timespec timestamp;  // Timestamp of when packet was sent
    char data[];                // Flexible array member for variable-size data
};