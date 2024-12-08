#ifndef CLIENT_H
#define CLIENT_H

void run_tcp_upload_test(char *address, int port, int duration);
void run_tcp_download_test(char *address, int port, int duration);
void run_udp_upload_test(char *address, int port, int duration);
void run_udp_download_test(char *address, int port, int duration);
void run_ping_test(char *address, int port, int size, int duration, int interval);

#endif