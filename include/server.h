#include "../include/shared.h"

#ifndef SERVER_H
#define SERVER_H

void start_server(int port);
void handle_tcp_upload(int client_sock);
void handle_tcp_download(int client_sock);
void handle_udp_upload(client_data_t* data);
void handle_ping(client_data_t* data);

#endif