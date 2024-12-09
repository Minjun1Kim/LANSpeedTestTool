#include "../include/server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define BUFFER_SIZE 1024
#define MAX_PACKETS 10

void handle_upload(int client_sock) {
    char buffer[BUFFER_SIZE];
    long total_bytes = 0;
    struct timeval start, end;

    gettimeofday(&start, NULL);
    while (1) {
        int bytes = recv(client_sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            break; // Client closed connection or error
        }
        total_bytes += bytes;
    }
    gettimeofday(&end, NULL);

    long time_diff = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_usec - start.tv_usec);
    double mbps = (total_bytes * 8.0) / time_diff / 1e6;
    printf("Upload Test: Received %ld bytes in %ld microseconds (~%.2f Mbps)\n",
           total_bytes, time_diff, mbps);
}

void handle_download(int client_sock, int size, int duration) {
    char *data = malloc(size);
    memset(data, 'A', size);

    struct timeval start, end;
    gettimeofday(&start, NULL);
    for (int i = 0; i < duration; i++) {
        if (send(client_sock, data, size, 0) < 0) {
            perror("Data send failed");
            break;
        }
    }
    gettimeofday(&end, NULL);

    long time_diff = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_usec - start.tv_usec);
    printf("Download Test: Sent %d bytes for %ld seconds\n", size * duration, time_diff);

    free(data);
}

void handle_ping(int client_sock) {
    char buffer[BUFFER_SIZE];
    while (1) {
        int bytes = recv(client_sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            break; // End of data or error
        }
        send(client_sock, buffer, bytes, 0); // Echo back the received data
    }
}

void handle_jitter(int client_sock, int num_packets, int packet_size) {
    char buffer[packet_size];
    struct timeval start, end;
    long *rtt_values = malloc(sizeof(long) * num_packets);
    int count = 0;

    memset(rtt_values, 0, sizeof(long) * num_packets);

    // Collect RTT samples
    while (count < num_packets) {
        gettimeofday(&start, NULL);

        // Receive a packet from the client
        int bytes_received = recv(client_sock, buffer, packet_size, 0);
        if (bytes_received <= 0) {
            break; // End of data or error
        }

        // Send echo back to the client
        if (send(client_sock, buffer, bytes_received, 0) < 0) {
            perror("Failed to send echo");
            break;
        }

        gettimeofday(&end, NULL);

        // Calculate RTT in microseconds
        rtt_values[count++] = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_usec - start.tv_usec);
    }

    // Calculate jitter
    long jitter = 0;
    for (int i = 1; i < count; i++) {
        jitter += labs(rtt_values[i] - rtt_values[i - 1]);
    }
    if (count > 1) {
        jitter /= (count - 1);
    }

    // Construct JSON response
    char json_response[BUFFER_SIZE * 4];
    snprintf(json_response, sizeof(json_response), "{ \"rtt_values\": [");

    for (int i = 0; i < count; i++) {
        char rtt_str[32];
        snprintf(rtt_str, sizeof(rtt_str), "%ld", rtt_values[i]);
        strcat(json_response, rtt_str);
        if (i < count - 1) {
            strcat(json_response, ",");
        }
    }

    strcat(json_response, "], \"jitter\": ");
    char jitter_str[32];
    snprintf(jitter_str, sizeof(jitter_str), "%ld", jitter);
    strcat(json_response, jitter_str);
    strcat(json_response, " }");

    // Send JSON response to the client
    if (send(client_sock, json_response, strlen(json_response), 0) < 0) {
        perror("Failed to send JSON response");
    }

    printf("Jitter Test: Sent JSON response: %s\n", json_response);

    free(rtt_values);
}

void start_server(int port) {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    char test_type[32];

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
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

    if (listen(server_sock, 5) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);

    addr_size = sizeof(client_addr);
    while ((client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size)) > 0) {
        printf("Client connected\n");

        memset(test_type, 0, sizeof(test_type));
        int bytes_received = recv(client_sock, test_type, sizeof(test_type) - 1, 0);
        if (bytes_received <= 0) {
            perror("Error reading test type");
            close(client_sock);
            continue;
        }

        test_type[bytes_received] = '\0';

        // Step 2: Send acknowledgment to client
        send(client_sock, "ACK", 3, 0);

        // receive packet_size and num_packets from the client (for jitter)
        int packet_size, num_packets;

        if (strcmp(test_type, "upload") == 0) {
            handle_upload(client_sock);
        } else if (strcmp(test_type, "download") == 0) {
            handle_download(client_sock, 1024, 10);
        } else if (strcmp(test_type, "ping") == 0) {
            handle_ping(client_sock);
        } else if (strcmp(test_type, "jitter") == 0) {
            // Receive packet_size
            if (recv(client_sock, &packet_size, sizeof(packet_size), 0) <= 0) {
                perror("Failed to receive packet_size for jitter test");
                close(client_sock);
                continue;
            }
            // Receive num_packets
            if (recv(client_sock, &num_packets, sizeof(num_packets), 0) <= 0) {
                perror("Failed to receive num_packets for jitter test");
                close(client_sock);
                continue;
            }

            handle_jitter(client_sock, num_packets, packet_size);
        } else {
            printf("Unknown test type: %s\n", test_type);
        }

        close(client_sock);
        printf("Client disconnected\n");
    }

    close(server_sock);
}
