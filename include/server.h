#ifndef SERVER_H
#define SERVER_H

void start_server(int port);
void handle_upload(int client_sock);
void handle_download(int client_sock, int size, int duration);
void handle_ping(int client_sock);
void handle_jitter(int client_sock, int num_packets, int packet_size);

#endif