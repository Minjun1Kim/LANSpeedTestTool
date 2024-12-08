#ifndef CLIENT_H
#define CLIENT_H

void run_upload_test(char *address, int port, int size, int duration);
void run_download_test(char *address, int port, int size, int duration);
void run_ping_test(char *address, int duration);
void run_jitter_test(char *address, int port, int packet_size, int num_packets);
void parse_json_response(const char *response, long *rtt_values, int *rtt_count, long *jitter);

#endif