#ifndef CLIENT_H
#define CLIENT_H

void run_upload_test(char *address, int port, int size, int duration);
void run_download_test(char *address, int port, int size, int duration);
void run_ping_test(char *address, int duration);
void run_jitter_test(char *address, int port, int size, int duration);

#endif