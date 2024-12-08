#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/server.h"
#include "../include/client.h"

void print_usage() {
    printf("Usage: lan_speed [options]\n");
    printf("Options:\n");
    printf("  -m, --mode       Mode of operation: server or client\n");
    printf("  -t, --test       Test type: upload, download, ping, jitter\n");
    printf("  -a, --address    Server address (for client mode)\n");
    printf("  -p, --port       Port number (default: 8080)\n");
    printf("  -s, --size       Packet size in bytes (default: 1024)\n");
    printf("  -n, --num        Number of packets (default: 10)\n");
    printf("  -d, --duration   Test duration in seconds (default: 10)\n");
    printf("  -h, --help       Display this help message\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    char *mode = NULL;
    char *test = NULL;
    char *address = NULL;
    int port = 8080;
    int size = 1024;
    int duration = 10;
    int num_packets = 10;

    int opt;
    while ((opt = getopt(argc, argv, "m:t:a:p:s:n:d:h")) != -1) {
        switch (opt) {
            case 'm': mode = optarg; break;
            case 't': test = optarg; break;
            case 'a': address = optarg; break;
            case 'p': port = atoi(optarg); break;
            case 's': size = atoi(optarg); break;
            case 'n': num_packets = atoi(optarg); break;
            case 'd': duration = atoi(optarg); break;
            case 'h':
            default: print_usage();
        }
    }

    if (!mode) {
        fprintf(stderr, "Error: Mode is required.\n");
        print_usage();
    }

    if (strcmp(mode, "server") == 0) {
        start_server(port);
    } else if (strcmp(mode, "client") == 0) {
        // Client mode requires both mode and test type
        if (!test || !address) {
            fprintf(stderr, "Error: Test type and server address are required for client mode.\n");
            print_usage();
        }

        // Handle the test type for client mode
        if (strcmp(test, "upload") == 0) {
            run_upload_test(address, port, size, duration);
        } else if (strcmp(test, "download") == 0) {
            run_download_test(address, port, size, duration);
        } else if (strcmp(test, "ping") == 0) {
            run_ping_test(address, duration);
        } else if (strcmp(test, "jitter") == 0) {
            run_jitter_test(address, port, size, num_packets);
        } else {
            fprintf(stderr, "Invalid test type: %s\n", test);
            print_usage();
        }
    } else {
        fprintf(stderr, "Invalid mode: %s\n", mode);
        print_usage();
    }

    return 0;
}

