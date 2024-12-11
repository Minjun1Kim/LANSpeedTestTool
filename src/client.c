#include "../include/client.h"
#include "../include/shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/ip_icmp.h>
#include <math.h>
#include <netdb.h>

static int create_tcp_socket(char *address, int port) {
    int client_sock;
    struct sockaddr_in server_addr;

    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, address, &server_addr.sin_addr) <= 0) {
        perror("Invalid server IP address");
        close(client_sock);
        exit(EXIT_FAILURE);
    }

    if (connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(client_sock);
        exit(EXIT_FAILURE);
    }

    struct timeval timeout;
    timeout.tv_sec = 2;  // Set timeout for 2 seconds
    timeout.tv_usec = 0;

    // Set socket timeout using setsockopt
    if (setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt failed");
        close(client_sock);
        exit(EXIT_FAILURE);
    }

    return client_sock;
}

static int create_udp_socket(char *address, int port, struct sockaddr_in *server_addr) {
    int server_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sock < 0) {
        perror("UDP Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);
    if (inet_pton(AF_INET, address, &server_addr->sin_addr) <= 0) {
        perror("Invalid server IP address");
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


void run_udp_upload_test(char *address, int port, int duration) {
    struct sockaddr_in server_addr;
    int server_sock = create_udp_socket(address, port, &server_addr);

    // Send test type
    if (sendto(server_sock, "upload", strlen("upload"), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to send test type");
        close(server_sock);
        return;
    }

    char *data = malloc(BUFFER_SIZE);
    memset(data, 'A', BUFFER_SIZE);

    struct timespec start_time, end_time;
    socklen_t addr_len = sizeof(server_addr);
    double total_elapsed_time = 0;

    while (total_elapsed_time < 0.1) {
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        long bytes_sent_i = sendto(server_sock, data, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, addr_len);
        if (bytes_sent_i < 0) {
                perror("Data send failed1");
                break;
            }
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        double time_taken = (end_time.tv_sec - start_time.tv_sec) +
                            (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
        total_elapsed_time += time_taken;
    }

    // Step 3: Start the upload test
    total_elapsed_time = 0;
    long bytes_sent = 0;
    int iteration = 1;
    while (total_elapsed_time < duration) {
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        long bytes_sent_i = sendto(server_sock, data, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, addr_len);
        if (bytes_sent_i < 0) {
                perror("Data send failed2");
                break;
            }
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        bytes_sent += bytes_sent_i;
        double time_taken = (end_time.tv_sec - start_time.tv_sec) +
                            (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

        total_elapsed_time += time_taken;
        if (total_elapsed_time >= iteration) {
            double iter_elapsed_time = total_elapsed_time - iteration + 1;
            double megabytes = (double)bytes_sent / (1024.0 * 1024.0);
            printf("Upload Test: Sent %.3f megabytes in %.4f seconds (~%.2f Mbps)\n",
                    megabytes, iter_elapsed_time, (bytes_sent * 8.0) / iter_elapsed_time / 1e6);
            iteration++;
            bytes_sent = 0;
        }
    }

    free(data);
    close(server_sock);
}

void run_udp_download_test(char *address, int port, int duration) {
    struct sockaddr_in server_addr;
    int server_sock = create_udp_socket(address, port, &server_addr);

    // Send test type
    if (sendto(server_sock, "download", 8, 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to send test type");
        close(server_sock);
        return;
    }

    char *buffer = malloc(BUFFER_SIZE);
    struct timespec start_time, end_time;
    socklen_t addr_len = sizeof(server_addr);
    double total_elapsed_time = 0;
    long bytes_recieved = 0;
    int iteration = 1;
    while (total_elapsed_time < duration) {
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        long bytes_recieved_i = recvfrom(server_sock, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &addr_len);
        if (bytes_recieved_i < 0) {
                perror("Data recieve failed");
                break;
            }
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        bytes_recieved += bytes_recieved_i;
        double time_taken = (end_time.tv_sec - start_time.tv_sec) +
                            (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
        total_elapsed_time += time_taken;
        if (total_elapsed_time >= iteration) {
            double iter_elapsed_time = total_elapsed_time - iteration + 1;
            double megabytes = (double)bytes_recieved / (1024.0 * 1024.0);
            printf("Download Test: Recieved %.3f bytes in %.4f seconds (~%.2f Mbps)\n",
                    megabytes, iter_elapsed_time, (bytes_recieved * 8.0) / iter_elapsed_time / 1e6);
            iteration++;
            bytes_recieved = 0;
        }
    }

    sendto(server_sock, "done", 5, 0, (struct sockaddr*)&server_addr, addr_len);

    free(buffer);
    close(server_sock);
}

void run_tcp_upload_test(char *address, int port, int duration) {
    int client_sock = create_tcp_socket(address, port);

    // Step 1: Send the test type
    send(client_sock, "upload", strlen("upload"), 0);

    char *data = malloc(BUFFER_SIZE);
    memset(data, 'A', BUFFER_SIZE);
    struct timespec start_time, end_time;
    double total_elapsed_time = 0;
    while (total_elapsed_time < 0.1) {
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        long bytes_sent_i = send(client_sock, data, BUFFER_SIZE, 0);
        if (bytes_sent_i < 0) {
                perror("Data send failed3");
                break;
            }
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        double time_taken = (end_time.tv_sec - start_time.tv_sec) +
                            (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
        total_elapsed_time += time_taken;
    }

    // Step 3: Start the upload test
    total_elapsed_time = 0;
    long bytes_sent = 0;
    int iteration = 1;
    while (total_elapsed_time < duration) {
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        long bytes_sent_i = send(client_sock, data, BUFFER_SIZE, 0);
        if (bytes_sent_i < 0) {
                perror("Data send failed4");
                break;
            }
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        bytes_sent += bytes_sent_i;
        double time_taken = (end_time.tv_sec - start_time.tv_sec) +
                            (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

        total_elapsed_time += time_taken;
        if (total_elapsed_time >= iteration) {
            double iter_elapsed_time = total_elapsed_time - iteration + 1;
            double megabytes = (double)bytes_sent / (1024.0 * 1024.0);

            printf("Upload Test: Sent %.2f MB in %.2f seconds (~%.2f MB/S)\n",
                    megabytes, iter_elapsed_time, (megabytes / iter_elapsed_time));
            iteration++;
            bytes_sent = 0;
        }
    }

    free(data);
    close(client_sock);
}


void run_tcp_download_test(char *address, int port, int duration) {
    int client_sock = create_tcp_socket(address, port);

    send(client_sock, "download", strlen("download"), 0);

    char *data = malloc(BUFFER_SIZE);
    struct timespec start_time, end_time;
    double total_elapsed_time = 0;
    long bytes_recieved = 0;
    int iteration = 1;
    while (total_elapsed_time < duration) {
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        long bytes_recieved_i = recv(client_sock, data, BUFFER_SIZE, 0);
        if (bytes_recieved_i < 0) {
                perror("Data recieve failed");
                break;
            }
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        bytes_recieved += bytes_recieved_i;
        double time_taken = (end_time.tv_sec - start_time.tv_sec) +
                            (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
        total_elapsed_time += time_taken;
        if (total_elapsed_time >= iteration) {
            double iter_elapsed_time = total_elapsed_time - iteration + 1;
            double megabytes = (double)bytes_recieved / (1024.0 * 1024.0);

            printf("Download Test: Recieved %.2f MB in %.6f seconds (~%.2f MB/S)\n",
                    megabytes, iter_elapsed_time, (megabytes / iter_elapsed_time));
            iteration++;
            bytes_recieved = 0;
        }
    }

    free(data);
    close(client_sock);
}

void run_ping_test(char *address, int port, int size, int duration, int interval) {
    struct timespec start_time, end_time;
    float packets_lost = 0.0;
    double rtts[duration];
    char *buffer = "ping";

    int server_sock;
    struct sockaddr_in server_addr;

    server_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, address, &server_addr.sin_addr) <= 0) {
        perror("Invalid server IP address");
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
    
    if (sendto(server_sock, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Send Ping test type failed");
        return;
    }

    if (sendto(server_sock, &size, sizeof(size), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Send packet size failed");
        return;
    }

    char data[size];
    memset(data, 'A', size);

    for (int i = 0; i < duration; i++) {
        sleep(interval);
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        
        // Send a ping message to the server
        if (sendto(server_sock, data, size, 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            packets_lost++;
            perror("Ping send failed");
            continue;
        }

        // Receive the ping reply from the server
        socklen_t addr_len = sizeof(server_addr);
        if (recvfrom(server_sock, data, size, 0, (struct sockaddr*)&server_addr, &addr_len) < 0) {
            packets_lost++;
            perror("Ping receive failed");
            continue;
        }
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        
        double rtt = (end_time.tv_sec - start_time.tv_sec) +
                            (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
        rtt *= 1000;
        rtts[i] = rtt;
        printf("Ping %d: RTT = %.4f ms\n", i + 1, rtt);
    }

    double jitter = 0;
    for (int i = 1; i < duration; i++) {   
        jitter += fabs(rtts[i] - rtts[i-1]);
    }
    jitter /= (duration - packets_lost - 1);
    printf("Jitter: %.4f\n", jitter);
    printf("Packet Loss: %.2f%%\n", (packets_lost/(float)duration)*100);

    close(server_sock);
}

void run_icmp_ping_test(char *address, int port, int size, int duration, int interval) {

    int server_sock;
    struct sockaddr_in server_addr;
    struct hostent *host;
    char send_packet[sizeof(struct icmphdr) + size];
    char recv_packet[BUFFER_SIZE];
    struct icmphdr *icmp_hdr;
    struct iphdr *recv_ip;
    int bytes_received;

    // Create raw socket for ICMP
    if ((server_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
        perror("ICMP socket creation failed. Need to run as root.");
        return; // Exit gracefully without calling exit()
    }

    // Set socket timeout (2 seconds)
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    if (setsockopt(server_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt failed");
        close(server_sock);
        return;
    }

    // Resolve host
    host = gethostbyname(address);
    if (!host) {
        fprintf(stderr, "Failed to resolve hostname: %s\n", address);
        close(server_sock);
        return;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    // While port is not used by ICMP, we keep it consistent.
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr, host->h_addr, host->h_length);

    // Prepare ICMP Echo Request
    icmp_hdr = (struct icmphdr*)send_packet;
    icmp_hdr->type = ICMP_ECHO;
    icmp_hdr->code = 0;
    icmp_hdr->un.echo.id = getpid();
    icmp_hdr->un.echo.sequence = 0;
    memset(send_packet + sizeof(struct icmphdr), 'A', size);
    icmp_hdr->checksum = 0;
    icmp_hdr->checksum = calculate_checksum((unsigned short*)send_packet, sizeof(send_packet));

    int sent_packets = 0;
    int received_packets = 0;
    double rtts[1000]; // Ensure 1000 is large enough for your duration
    int max_rtts = (int)(sizeof(rtts) / sizeof(rtts[0]));

    for (int i = 0; i < duration; i++) {
        // Increment sequence number each iteration
        icmp_hdr->un.echo.sequence = i + 1;
        icmp_hdr->checksum = 0;
        icmp_hdr->checksum = calculate_checksum((unsigned short*)send_packet, sizeof(send_packet));

        struct timeval send_time, recv_time;
        gettimeofday(&send_time, NULL);

        // Send ICMP Echo Request
        if (sendto(server_sock, send_packet, sizeof(send_packet), 0, 
                   (struct sockaddr*)&server_addr, sizeof(server_addr)) <= 0) {
            perror("Ping send failed");
            // Don't break here, continue to next iteration
            sleep(interval);
            continue;
        }
        sent_packets++;

        socklen_t addr_len = sizeof(server_addr);
        bytes_received = recvfrom(server_sock, recv_packet, sizeof(recv_packet), 0,
                                  (struct sockaddr*)&server_addr, &addr_len);
        if (bytes_received <= 0) {
            printf("Ping %d: Request timed out.\n", i + 1);
            sleep(interval);
            continue;
        }

        gettimeofday(&recv_time, NULL);

        // Parse received packet
        recv_ip = (struct iphdr*)recv_packet;
        int ip_header_len = recv_ip->ihl * 4;
        struct icmphdr *recv_icmp = (struct icmphdr*)(recv_packet + ip_header_len);

        if (recv_icmp->type == ICMP_ECHOREPLY && recv_icmp->un.echo.id == getpid()) {
            double rtt = (recv_time.tv_sec - send_time.tv_sec) +
                         (recv_time.tv_usec - send_time.tv_usec)/1e6;
            rtt *= 1000.0; // Convert to milliseconds
            if (received_packets < max_rtts) {
                rtts[received_packets++] = rtt;
            }
            printf("Ping %d: RTT = %.4f ms\n", i + 1, rtt);
        } else {
            printf("Ping %d: Received non-echo reply or mismatched ID.\n", i + 1);
        }

        sleep(interval);
    }

    // Calculate Jitter
    double jitter = 0.0;
    if (received_packets > 1) {
        for (int i = 1; i < received_packets; i++) {
            jitter += fabs(rtts[i] - rtts[i-1]);
        }
        jitter /= (received_packets - 1);
    }

    printf("Jitter: %.4f ms\n", jitter);
    double packet_loss = (sent_packets == 0) ? 0.0 : ((double)(sent_packets - received_packets)/sent_packets)*100.0;
    printf("Packet Loss: %.2f%%\n", packet_loss);

    close(server_sock);
}
