#include "../include/server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdio.h>

#define BUFFER_SIZE 32768

int create_socket(int type, int port) {
    int server_sock;
    struct sockaddr_in server_addr;

    server_sock = socket(AF_INET, type, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
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

    struct timeval timeout;
    timeout.tv_sec = 2;  // Set timeout for 2 seconds
    timeout.tv_usec = 0;

    // Set socket timeout using setsockopt
    if (setsockopt(server_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt failed");
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

    long time_diff = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_usec - start.tv_usec);
    printf("Upload Test: Received %ld bytes in %ld microseconds (~%.2f Mbps)\n",
           total_bytes, time_diff, (total_bytes * 8.0) / time_diff / 1e6);
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
    printf("Download Test: Sent %ld bytes for %ld seconds\n", BUFFER_SIZE * time_diff, time_diff);

    free(data);
}

void handle_udp_upload(client_data_t* data) {

}
void handle_udp_download(client_data_t* data) {

}

void handle_ping(client_data_t* data) {
    int packet_size;
    int len = recvfrom(data->sockfd, &packet_size, sizeof(packet_size), 0, 
                           (struct sockaddr *)&data->client_addr, &data->addr_len);
    if (len < 0) {
        perror("Receive failed1");
        return;
    }

    char buffer[packet_size];
    while (1) {
        memset(buffer, 0, packet_size);
        len = recvfrom(data->sockfd, buffer, packet_size, 0, 
                           (struct sockaddr *)&data->client_addr, &data->addr_len);
        if (len < 0) {
            perror("Receive failed2");
            break;
        }

        printf("Received packet from client\n");
        if (sendto(data->sockfd, buffer, len, 0, 
                   (struct sockaddr *)&data->client_addr, data->addr_len) < 0) {
            perror("Send failed");
            break;
        }
    }
}

static void *handle_tcp_client(void* arg) {
    int client_sock = *((int*)arg);

    printf("Client connected\n");

    char test_type[10]; 
    memset(test_type, 0, sizeof(test_type));
    int bytes_received = recv(client_sock, test_type, sizeof(test_type) - 1, 0); // Read only the test type
    if (bytes_received <= 0) {
        perror("Error reading test type");
        close(client_sock);
        return;
    }

    test_type[bytes_received] = '\0';

    // Step 3: Handle the requested test
    if (strcmp(test_type, "upload") == 0) {
        handle_tcp_upload(client_sock);
    } else if (strcmp(test_type, "download") == 0) {
        handle_tcp_download(client_sock);
    } else {
        printf("Unknown test type: %s\n", test_type);
    }

    close(client_sock);
    printf("Client disconnected\n");
}

static void *handle_udp_client(void* arg) {
    client_data_t* client_data = ((client_data_t*)arg);

    printf("Client connected\n");

    if (sendto(client_data->sockfd, "ack", 4, 0, (struct sockaddr*)&client_data->client_addr, sizeof(client_data->client_addr)) < 0) {
        perror("Send Ack failed");
        return;
    }

    if (strcmp(client_data->test, "upload")) {
        handle_udp_upload(client_data);
    } else if (strcmp(client_data->test, "download")) {
        handle_udp_download(client_data);
    } else if (strcmp(client_data->test, "ping")) {
        handle_ping(client_data);
    } else {
        printf("Unknown test type: %s\n", client_data->test);
    }
}

static void *start_tcp_thread(void* arg) {
    int server_sock = *((int*)arg);
    socklen_t addr_size;
    struct sockaddr_in client_addr;

    addr_size = sizeof(client_addr);
    while (1) {
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, &handle_tcp_client, (void*)&client_sock) != 0) {
            perror("Failed to create tcp client thread");
        }
    }

    close(server_sock);
}

static void *start_udp_thread(void* arg) {
    int server_sock = *((int*)arg);

    while (1) {
        client_data_t *client_data = malloc(sizeof(client_data_t));
        if (!client_data) {
            perror("Malloc failed");
            continue;
        }

        client_data->sockfd = server_sock;
        client_data->addr_len = sizeof(client_data->client_addr);

        memset(client_data->test, 0, 10);
        int len = recvfrom(server_sock, client_data->test, 10, 0, (struct sockaddr *)&client_data->client_addr, &client_data->addr_len);
        if (len < 0) {
            perror("Receive failed3");
            free(client_data);
            continue;
        }

        printf("Connection established with client.\n");

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, &handle_udp_client, client_data) != 0) {
            perror("Thread creation failed");
            free(client_data);
            continue;
        }
    }

    close(server_sock);
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
        perror("Failed to create server thread");
    }
    if (pthread_create(&udp_thread, NULL, start_udp_thread, (void*)&udp_sock) != 0) {
        perror("Failed to create server thread");
    }

    pthread_join(tcp_thread, NULL);
    pthread_join(udp_thread, NULL);

    close(tcp_sock);
    close(udp_sock);
}