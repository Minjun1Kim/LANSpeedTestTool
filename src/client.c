#include "../include/client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

// calculate the difference between two timevals
long calculate_time_diff(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_usec - start.tv_usec);
}


void run_upload_test(char *address, int port, int size, int duration) {
    int client_sock;
    struct sockaddr_in server_addr;
    char *data = malloc(size);
    char ack[32];
    memset(data, 'A', size);
    struct timeval start, end;

    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        perror("Socket creation failed");
        free(data);
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, address, &server_addr.sin_addr);

    if (connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        free(data);
        close(client_sock);
        exit(EXIT_FAILURE);
    }

    // Step 1: Send the test type
    send(client_sock, "upload", strlen("upload"), 0);

    // Step 2: Wait for acknowledgment
    int ack_received = recv(client_sock, ack, sizeof(ack) - 1, 0);
    if (ack_received <= 0 || strncmp(ack, "ACK", 3) != 0) {
        perror("Server did not acknowledge test type");
        free(data);
        close(client_sock);
        return;
    }

    // Step 3: Start the upload test
    gettimeofday(&start, NULL);
    long bytes_sent = 0;
    for (int i = 0; i < duration; i++) {
        if (send(client_sock, data, size, 0) < 0) {
            perror("Data send failed");
            break;
        }
        bytes_sent += size;
    }
    gettimeofday(&end, NULL);

    long time_diff = calculate_time_diff(start, end);
    printf("Upload Test: Sent %ld bytes in %ld microseconds (~%.2f Mbps)\n",
           bytes_sent, time_diff, (bytes_sent * 8.0) / time_diff / 1e6);

    free(data);
    close(client_sock);
}


void run_download_test(char *address, int port, int size, int duration) {
    int client_sock;
    struct sockaddr_in server_addr;
    char *data = malloc(size);
    struct timeval start, end;

    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        perror("Socket creation failed");
        free(data);
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, address, &server_addr.sin_addr);

    if (connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        free(data);
        close(client_sock);
        exit(EXIT_FAILURE);
    }

    send(client_sock, "download", strlen("download"), 0);

    gettimeofday(&start, NULL);
    long bytes_received = 0;
    for (int i = 0; i < duration; i++) {
        int bytes = recv(client_sock, data, size, 0);
        if (bytes <= 0) {
            perror("Data receive failed");
            break;
        }
        bytes_received += bytes;
    }
    gettimeofday(&end, NULL);

    long time_diff = calculate_time_diff(start, end);
    printf("Upload Test: Received %ld bytes in %ld microseconds (~%.2f Mbps)\n",
       bytes_received, time_diff, 
       (bytes_received * 8.0) / (time_diff > 0 ? time_diff : 1) / 1e6);

    free(data);
    close(client_sock);
}

void run_ping_test(char *address, int duration) {
    struct sockaddr_in server_addr;
    struct timeval start, end;
    int sock;
    char buffer[64] = "Ping test message";

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345); // Arbitrary port for UDP ping
    inet_pton(AF_INET, address, &server_addr.sin_addr);

    send(sock, "ping", strlen("ping"), 0);

    for (int i = 0; i < duration; i++) {
        gettimeofday(&start, NULL);
        if (sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("Ping send failed");
            break;
        }

        socklen_t addr_len = sizeof(server_addr);
        if (recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&server_addr, &addr_len) < 0) {
            perror("Ping receive failed");
            break;
        }
        gettimeofday(&end, NULL);

        long time_diff = calculate_time_diff(start, end);
        printf("Ping %d: RTT = %ld microseconds\n", i + 1, time_diff);
    }

    close(sock);
}

void run_jitter_test(char *address, int port, int size, int duration) {
    int client_sock;
    struct sockaddr_in server_addr;
    char *data = malloc(size);
    struct timeval start, end;
    long rtt_values[duration];
    int count = 0;

    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        perror("Socket creation failed");
        free(data);
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, address, &server_addr.sin_addr);

    if (connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        free(data);
        close(client_sock);
        exit(EXIT_FAILURE);
    }

    send(client_sock, "jitter", strlen("jitter"), 0);

    for (int i = 0; i < duration; i++) {
        gettimeofday(&start, NULL);
        if (send(client_sock, data, size, 0) < 0) {
            perror("Data send failed");
            break;
        }
        if (recv(client_sock, data, size, 0) <= 0) {
            perror("Data receive failed");
            break;
        }
        gettimeofday(&end, NULL);

        rtt_values[count++] = calculate_time_diff(start, end);
    }

    long jitter = 0;
    for (int i = 1; i < count; i++) {
        jitter += abs(rtt_values[i] - rtt_values[i - 1]);
    }
    jitter /= (count - 1);

    printf("Jitter Test: Average jitter = %ld microseconds\n", jitter);

    free(data);
    close(client_sock);
}
