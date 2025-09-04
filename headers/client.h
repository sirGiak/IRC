//
// Created by giacomo on 29/08/25.
//

#ifndef CLIENT_H
#define CLIENT_H

#define SERVER_IP "127.0.0.1"

//client.c
void *read_IRC_replies();

//utils_client.c
int start_tcp_connection(struct sockaddr_in *serv_addr);
void print_help();
void analyze_response(char *buffer);

#endif //CLIENT_H
