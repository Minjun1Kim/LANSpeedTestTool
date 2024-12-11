#include "../include/server.h"
#include "../include/shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <signal.h>
#include <glib.h>

#define BUFFER_SIZE 32768

guint sockaddr_in_hash(gconstpointer key) {
    const struct sockaddr_in *addr = (const struct sockaddr_in*)key;
    guint hash = 0;
    // Combine the hash of the IP address and port
    hash = g_int_hash(&addr->sin_addr.s_addr);  // Hash the IP address
    hash = hash * 31 + g_int_hash(&addr->sin_port);  // Hash the port
    return hash;
}

// Custom equality function for struct sockaddr_in
gboolean sockaddr_in_equal(gconstpointer a, gconstpointer b) {
    const struct sockaddr_in *addr1 = (const struct sockaddr_in*)a;
    const struct sockaddr_in *addr2 = (const struct sockaddr_in*)b;
    // Compare both IP address and port
    return (addr1->sin_addr.s_addr == addr2->sin_addr.s_addr) &&
           (addr1->sin_port == addr2->sin_port);
}

static void update_hash_table(GHashTable *table, struct sockaddr_in *key, int value) {
    int *val = malloc(sizeof(int));
    if (!val) {
        perror("Memory allocation failed: val");
        return;
    }
    *val = value;
    g_hash_table_insert(table, key, val);
}

int create_socket(int type, int port) {
    int server_sock;
    struct sockaddr_in server_addr;

    server_sock = socket(AF_INET, type, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    return server_sock;
}

void handle_icmp_ping(int icmp_sock) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    while (1) {
        // Receive ICMP Echo Request
        int bytes_received = recvfrom(icmp_sock, buffer, sizeof(buffer), 0,
                                      (struct sockaddr *)&client_addr, &addr_len);
        if (bytes_received < 0) {
            if (errno == EINTR) continue; // Handle interrupt signal gracefully
            perror("ICMP Receive failed");
            continue;
        }

        struct iphdr *ip_hdr = (struct iphdr *)buffer;
        int ip_header_len = ip_hdr->ihl * 4;
        struct icmphdr *icmp_hdr = (struct icmphdr *)(buffer + ip_header_len);

        if (icmp_hdr->type == ICMP_ECHO) {
            // Prepare ICMP Echo Reply
            icmp_hdr->type = ICMP_ECHOREPLY;
            icmp_hdr->checksum = 0;
            icmp_hdr->checksum = calculate_checksum((unsigned short *)icmp_hdr, bytes_received - ip_header_len);

            // Send ICMP Echo Reply
            if (sendto(icmp_sock, icmp_hdr, bytes_received - ip_header_len, 0,
                       (struct sockaddr *)&client_addr, addr_len) < 0) {
                perror("ICMP Send failed");
            } else {
                printf("ICMP Echo Reply sent to %s\n", inet_ntoa(client_addr.sin_addr));
            }
        }
    }
}

void handle_tcp_upload(int client_sock) {
    char buffer[BUFFER_SIZE/4];
    long total_bytes = 0;
    struct timeval start, end;

    gettimeofday(&start, NULL);
    while (1) {
        int bytes = recv(client_sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            break; // End of data or error
        }
        total_bytes += bytes;
    }
    gettimeofday(&end, NULL);

    long time_diff = (end.tv_sec - start.tv_sec)*1000000L+(end.tv_usec - start.tv_usec);
    double mbps = 0.0;
    if (time_diff > 0) {
        mbps = (total_bytes * 8.0) / time_diff;
    }

    printf("TCP Upload Test: Received %ld bytes in %ld microseconds (~%.2f Mbps)\n",
           total_bytes, time_diff, mbps);
}

void handle_tcp_download(int client_sock) {
    char *data = malloc(BUFFER_SIZE);
    memset(data, 'A', BUFFER_SIZE);

    struct timeval start, end;
    
    gettimeofday(&start, NULL);
    while (1) {
        if (send(client_sock, data, BUFFER_SIZE, 0) < 0) {
            perror("TCP download: Send failed");
            break;
        }
    }
    gettimeofday(&end, NULL);

    long time_diff = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_usec - start.tv_usec);
    printf("Download Test: Sent %ld bytes for %ld seconds\n", BUFFER_SIZE * time_diff, time_diff);

    free(data);
}

void *handle_udp_download(void* arg) {
    client_data_t* data = (client_data_t*)arg;
    // For UDP download: we continuously send data until client stops or duration passes.
    // Client can measure how much it got.
    char *packet = malloc(BUFFER_SIZE);
    memset(packet, 'A', BUFFER_SIZE);

    // Just send data for a while; assume client times out after duration
    // In a more robust design, you'd signal when to stop.
    int stop = 0;
    while (stop == 0) {
        if (sendto(data->sockfd, packet, BUFFER_SIZE, 0,
                   (struct sockaddr*)&data->client_addr, data->addr_len) < 0) {
            perror("UDP Download: Send failed");
            break;
        }
        int *val = (int*)g_hash_table_lookup(data->thread_table, &data->client_addr);
        stop = *val;
    }

    printf("UDP Download Test completed sending.\n");
    free(packet);
    return NULL;
}

static void *handle_tcp_client(void* arg) {
    int client_sock = *((int*)arg);
    free(arg);
    printf("TCP Client connected\n");

    char test_type[32]; 
    memset(test_type, 0, sizeof(test_type));
    int bytes_received = recv(client_sock, test_type, sizeof(test_type) - 1, 0);
    if (bytes_received <= 0) {
        perror("Error reading test type");
        close(client_sock);
        return NULL;
    }

    test_type[bytes_received] = '\0';

    if (strcmp(test_type, "upload") == 0) {
        handle_tcp_upload(client_sock);
    } else if (strcmp(test_type, "download") == 0) {
        handle_tcp_download(client_sock);
    } else {
        printf("Unknown TCP test type: %s\n", test_type);
    }

    close(client_sock);
    printf("TCP Client disconnected\n");
    return NULL;
}


void* start_icmp_thread() {
    int icmp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (icmp_sock < 0) {
        perror("ICMP socket creation failed. Run as root.");
        pthread_exit(NULL);
    }

    printf("ICMP Ping Handler started.\n");
    handle_icmp_ping(icmp_sock);

    close(icmp_sock);
    pthread_exit(NULL);
}

static void *start_tcp_thread(void* arg) {
    int server_sock = *((int*)arg);
    socklen_t addr_size;
    struct sockaddr_in client_addr;

    addr_size = sizeof(client_addr);
    while (1) {
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        int *csock_ptr = malloc(sizeof(int));
        *csock_ptr = client_sock;
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, &handle_tcp_client, (void*)csock_ptr) != 0) {
            perror("Failed to create tcp client thread");
            close(client_sock);
            free(csock_ptr);
        }
        pthread_detach(thread_id);
    }

    close(server_sock);
    return NULL;
}

static void *start_udp_thread(void* arg) {
    int server_sock = *((int*)arg);
    GHashTable* thread_table = g_hash_table_new(sockaddr_in_hash, sockaddr_in_equal);
    char buffer[BUFFER_SIZE];
    client_data_t *client_data = malloc(sizeof(client_data_t));
    if (!client_data) {
            perror("Malloc failed");
            return NULL;
        }
    client_data->sockfd = server_sock;
    client_data->addr_len = sizeof(client_data->client_addr);
    client_data->thread_table = thread_table;

    while (1) {
        int len = recvfrom(server_sock, buffer, BUFFER_SIZE, 0,
                           (struct sockaddr *)&client_data->client_addr, &client_data->addr_len);
        if (len < 0) {
            perror("Receive failed");
            free(client_data);
            continue;
        }
        buffer[len] = '\0';

        if (strncmp(buffer, "download", strlen("download")) == 0) {
            printf("UDP request: %s\n", buffer);

            pthread_t thread_id;
            if (pthread_create(&thread_id, NULL, &handle_udp_download, client_data) != 0) {
                perror("Thread creation failed");
                free(client_data);
                continue;
            }
            pthread_detach(thread_id);

            int value = 0;
            update_hash_table(thread_table, &client_data->client_addr, value);
        } else if (strncmp(buffer, "done", strlen("done")) == 0) {
            int value = 1;
            update_hash_table(thread_table, &client_data->client_addr, value);
        } else if (strncmp(buffer, "ping", strlen("ping")) == 0) {
            int value = 2;
            update_hash_table(thread_table, &client_data->client_addr, value);
        } else if (*((int*)g_hash_table_lookup(client_data->thread_table, &client_data->client_addr)) == 2) {

            int value;
            memcpy(&value, buffer, sizeof(int));
            update_hash_table(thread_table, &client_data->client_addr, value);
        } else if (*((int*)g_hash_table_lookup(client_data->thread_table, &client_data->client_addr)) > 2) {
            sendto(server_sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_data->client_addr, client_data->addr_len);
        }
    }

    close(server_sock);
    return NULL;
}

void start_server(int port) {
    int udp_sock = create_socket(SOCK_DGRAM, port);
    int tcp_sock = create_socket(SOCK_STREAM, port);

    if (listen(tcp_sock, 5) < 0) {
        perror("Listen failed");
        close(tcp_sock);
        close(udp_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);

    pthread_t tcp_thread;
    pthread_t udp_thread;
    pthread_t icmp_thread;

    if (pthread_create(&tcp_thread, NULL, start_tcp_thread, (void*)&tcp_sock) != 0) {
        perror("Failed to create TCP server thread");
    }
    if (pthread_create(&udp_thread, NULL, start_udp_thread, (void*)&udp_sock) != 0) {
        perror("Failed to create UDP server thread");
    }
    if (pthread_create(&icmp_thread, NULL, start_icmp_thread, NULL) != 0) {
        perror("Failed to create ICMP server thread");
    }

    pthread_join(tcp_thread, NULL);
    pthread_join(udp_thread, NULL);
    pthread_join(icmp_thread, NULL);

    close(tcp_sock);
    close(udp_sock);
}