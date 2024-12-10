#include "../include/server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <errno.h>

#define BUFFER_SIZE 32768

int create_socket(int type, int port) {
    int server_sock;
    struct sockaddr_in server_addr;

    server_sock = socket(AF_INET, type, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
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
            perror("Data send failed");
            break;
        }
    }
    gettimeofday(&end, NULL);

    long time_diff = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_usec - start.tv_usec);
    printf("Download Test: Sent %ld bytes for %ld seconds\n", (long)BUFFER_SIZE * time_diff, time_diff);

    free(data);
}

void handle_udp_upload(client_data_t* data) {
    char buffer[BUFFER_SIZE];
    long total_bytes = 0;
    struct timeval start, end;
    gettimeofday(&start, NULL);

    while (1) {
        int bytes = recvfrom(data->sockfd, buffer, sizeof(buffer), 0,
                             (struct sockaddr *)&data->client_addr, &data->addr_len);
        if (bytes <= 0) {
            // possibly timeout or client done
            break;
        }
        total_bytes += bytes;
    }

    gettimeofday(&end, NULL);
    long time_diff = (end.tv_sec - start.tv_sec)*1000000L+(end.tv_usec - start.tv_usec);
    double mbps = 0.0;
    if (time_diff > 0) {
        mbps = (total_bytes * 8.0) / time_diff;
    }
    printf("UDP Upload Test: Received %ld bytes in %ld microseconds (~%.2f Mbps)\n",
           total_bytes, time_diff, mbps);
    free(data);
}

void handle_udp_download(client_data_t* data) {
    char *packet = malloc(BUFFER_SIZE);
    memset(packet, 'A', BUFFER_SIZE);

    struct timeval start, now;
    gettimeofday(&start, NULL);
    while (1) {
        if (sendto(data->sockfd, packet, BUFFER_SIZE, 0,
                   (struct sockaddr*)&data->client_addr, data->addr_len) < 0) {
            perror("UDP send failed");
            break;
        }
        gettimeofday(&now, NULL);
        long elapsed = (now.tv_sec - start.tv_sec)*1000000L+(now.tv_usec - start.tv_usec);
        if (elapsed > 5000000L) { // 5 seconds
            break;
        }
    }

    printf("UDP Download Test completed sending.\n");
    free(packet);
    free(data);
}

void handle_ping(client_data_t* data) {
    int packet_size;
    int len = recvfrom(data->sockfd, &packet_size, sizeof(packet_size), 0,
                       (struct sockaddr *)&data->client_addr, &data->addr_len);
    if (len < 0) {
        perror("Receive failed");
        free(data);
        return;
    }

    char buffer[packet_size];
    while (1) {
        memset(buffer, 0, packet_size);
        len = recvfrom(data->sockfd, buffer, packet_size, 0,
                       (struct sockaddr *)&data->client_addr, &data->addr_len);
        if (len <= 0) {
            break;
        }

        if (sendto(data->sockfd, buffer, len, 0,
                   (struct sockaddr *)&data->client_addr, data->addr_len) < 0) {
            perror("Send failed");
            break;
        }
    }

    printf("Ping test ended.\n");
    free(data);
}

static void *handle_udp_client(void* arg) {
    client_data_t* client_data = (client_data_t*)arg;
    printf("Client connected (UDP dedicated socket)\n");

    // Now we can safely send ack from this dedicated socket
    if (sendto(client_data->sockfd, "ack", 4, 0, 
               (struct sockaddr*)&client_data->client_addr, client_data->addr_len) < 0) {
        perror("Send Ack failed");
        free(client_data);
        return NULL;
    }

    if (strcmp(client_data->test, "upload") == 0) {
        handle_udp_upload(client_data);
    } else if (strcmp(client_data->test, "download") == 0) {
        handle_udp_download(client_data);
    } else if (strcmp(client_data->test, "ping") == 0) {
        handle_ping(client_data);
    } else {
        printf("Unknown test type: %s\n", client_data->test);
        free(client_data);
    }

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

    while (1) {
        client_data_t *client_data = malloc(sizeof(client_data_t));
        if (!client_data) {
            perror("Malloc failed");
            continue;
        }

        client_data->sockfd = server_sock; // temporarily
        client_data->addr_len = sizeof(client_data->client_addr);

        memset(client_data->test, 0, sizeof(client_data->test));
        int len = recvfrom(server_sock, client_data->test, sizeof(client_data->test)-1, 0,
                           (struct sockaddr *)&client_data->client_addr, &client_data->addr_len);
        if (len < 0) {
            perror("Receive failed");
            free(client_data);
            continue;
        }

        client_data->test[len] = '\0';
        printf("UDP request: %s\n", client_data->test);

        // Create a new socket for this client
        int client_sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (client_sock < 0) {
            perror("Failed to create client-specific UDP socket");
            free(client_data);
            continue;
        }

        struct sockaddr_in temp_addr;
        memset(&temp_addr, 0, sizeof(temp_addr));
        temp_addr.sin_family = AF_INET;
        temp_addr.sin_port = 0; // ephemeral port
        temp_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(client_sock, (struct sockaddr*)&temp_addr, sizeof(temp_addr)) < 0) {
            perror("Bind failed for client-specific socket");
            close(client_sock);
            free(client_data);
            continue;
        }

        socklen_t temp_len = sizeof(temp_addr);
        if (getsockname(client_sock, (struct sockaddr*)&temp_addr, &temp_len) < 0) {
            perror("getsockname failed");
            close(client_sock);
            free(client_data);
            continue;
        }

        unsigned short new_port = ntohs(temp_addr.sin_port);

        // Send the new port number to the client so it knows where to send subsequent packets
        if (sendto(server_sock, &new_port, sizeof(new_port), 0,
                   (struct sockaddr*)&client_data->client_addr, client_data->addr_len) < 0) {
            perror("Failed to send new port");
            close(client_sock);
            free(client_data);
            continue;
        }

        // Update client_data to use this new socket
        client_data->sockfd = client_sock;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, &handle_udp_client, client_data) != 0) {
            perror("Thread creation failed");
            close(client_sock);
            free(client_data);
            continue;
        }
        pthread_detach(thread_id);
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
    if (pthread_create(&tcp_thread, NULL, start_tcp_thread, (void*)&tcp_sock) != 0) {
        perror("Failed to create tcp server thread");
    }
    if (pthread_create(&udp_thread, NULL, start_udp_thread, (void*)&udp_sock) != 0) {
        perror("Failed to create udp server thread");
    }

    pthread_join(tcp_thread, NULL);
    pthread_join(udp_thread, NULL);

    close(tcp_sock);
    close(udp_sock);
}
