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
    printf("  -t, --test       Test type: upload, download, ping\n");
    printf("  -r, --protocol   Protocol used for tests:\n");
    printf("                   For upload/download: tcp or udp (default: tcp)\n");
    printf("                   For ping: udp or icmp (default: udp)\n");
    printf("  -s, --size       Packet size in bytes for ping test (default: 64)\n");
    printf("  -d, --duration   Test duration in seconds (packets number for ping) (default: 10)\n");
    printf("  -i, --interval   Interval Between Pings in Seconds (default: 1)\n");
    printf("  -h, --help       Display this help message\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    char *mode = NULL;
    char *test = NULL;
    char *protocol = "tcp";
    char *address = NULL;
    int port = 8080;
    int size = 64;
    int duration = 10;
    int interval = 1;

    int opt;
    while ((opt = getopt(argc, argv, "m:t:r:a:p:s:d:i:h")) != -1) {
        switch (opt) {
            case 'm': mode = optarg; break;
            case 't': test = optarg; break;
            case 'r': protocol = optarg; break;
            case 'a': address = optarg; break;
            case 'p': port = atoi(optarg); break;
            case 's': size = atoi(optarg); break;
            case 'd': duration = atoi(optarg); break;
            case 'i': interval = atoi(optarg); break;
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

        if (strcmp(test, "ping") == 0) {
            if (strcmp(protocol, "udp") != 0 && strcmp(protocol, "icmp") != 0) {
                fprintf(stderr, "Error: Invalid protocol for ping. Use udp or icmp.\n");
                print_usage();
            }
        } else {
            if (strcmp(protocol, "tcp") != 0 && strcmp(protocol, "udp") != 0) {
                fprintf(stderr, "Error: Invalid protocol for upload/download. Use tcp or udp.\n");
                print_usage();
            }
        }

        if (size < 4) {
            fprintf(stderr, "Error: Packet size must be at least 4 bytes.\n");
        }

        // Handle the test type for client mode
        if (strcmp(test, "upload") == 0) {
            if (strcmp(protocol, "tcp") == 0) {
                run_tcp_upload_test(address, port, duration);
            }
            else {
                run_udp_upload_test(address, port, duration);
            }
        } else if (strcmp(test, "download") == 0) {
            if (strcmp(protocol, "tcp") == 0) {
                run_tcp_download_test(address, port, duration);
            }
            else {
                run_udp_download_test(address, port, duration);
            }
        } else if (strcmp(test, "ping") == 0) {
             if (strcmp(protocol, "icmp") == 0) {
                run_icmp_ping_test(address, port, size, duration, interval);
            } else {
                run_ping_test(address, port, size, duration, interval);
            }
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

