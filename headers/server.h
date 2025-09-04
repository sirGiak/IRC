//
// Created by giacomo on 26/08/25.
//

#ifndef SERVER_H
#define SERVER_H

#define MAX_NUM 10 // massimo numero di thread
#define MAX_SOCKET 1020

typedef struct {
    int sockfd;
    char buffer[BUFFER_SIZE];
}message;


//utils_server.c
int init_socket_TCP(struct sockaddr_in *serv_addr, int backlog, unsigned int port);
void set_nonblocking(int sockfd);
void send_response(int sockfd, char response[]);
void init_fd_set(fd_set *read_fds, int array_sockfd[], int size);
int accept_new_connection_by_select(int *array_sockfd, int *size_array, struct sockaddr_in *cli_addr, socklen_t *clilen);
void close_connection_by_select(int *array_sockfd, int *size_array, int *index);
void close_all_connections_by_select(int *array_sockfd, int size_array);

//service_server.c
void nick(int sockfd, char* nickname);
void join(int sockfd, char* channel_names);
void part(int sockfd, char* channel_names);
void topic(int sockfd, char* parameters);
void quit(int sockfd);
void privmsg(int sockfd, char* destinations, char* text);


#endif //SERVER_H
