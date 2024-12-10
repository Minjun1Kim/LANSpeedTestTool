#include <arpa/inet.h>
#include <time.h>

typedef struct {
    int sockfd;
    struct sockaddr_in client_addr;
    socklen_t addr_len;
    char test[10];
} client_data_t;

struct packet {
    struct timespec timestamp;  // Timestamp of when packet was sent
    size_t length;
    char data[];                // Flexible array member for variable-size data
};