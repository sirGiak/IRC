#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include "headers/common.h"


int init_socket_TCP(struct sockaddr_in *serv_addr, int backlog, unsigned int port) {
    int sockfd;
    // Crea il socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // Configura l'indirizzo del server
    serv_addr->sin_family = AF_INET;
    serv_addr->sin_addr.s_addr = INADDR_ANY;
    serv_addr->sin_port = htons(port);
    // Collega il socket alla porta
    if (bind(sockfd, (struct sockaddr *)serv_addr, sizeof(*serv_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    // Mette il socket in ascolto
    if (listen(sockfd, backlog) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void send_response(int sockfd, char response[]) {
    char buffer[BUFFER_SIZE];
    printf("Send -> %s\n",response);
    sprintf(buffer, "%s\r\n", response);
    write(sockfd, buffer, strlen(buffer));
}

void set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

void init_fd_set(fd_set *read_fds, int array_sockfd[], int size){
    FD_ZERO(read_fds);

    for (int i=0; i<size; i++){
        FD_SET(array_sockfd[i], read_fds);
    }
}


int accept_new_connection_by_select(int *array_sockfd, int *size_array, struct sockaddr_in *cli_addr, socklen_t *clilen) {
    int newsockfd = accept(array_sockfd[0], (struct sockaddr *)cli_addr, clilen);

    if (newsockfd < 0) {
        perror("accept failed\n");
    }else {
        printf("Nuovo client connesso, fd: %d.\n", newsockfd);
        set_nonblocking(newsockfd); // Client socket non bloccante
        array_sockfd[*size_array]=newsockfd;
        *size_array = *size_array + 1;
    }
    return newsockfd;
}

void close_connection_by_select(int *array_sockfd, int *size_array, int *index) {
    int i=*index;
    close(array_sockfd[*index]);
    printf("Client disconnesso. FD: %d\n", array_sockfd[*index]);

    //shift di una posizione verso sinistra
    while(i<*size_array){
        array_sockfd[i]=array_sockfd[i+1];
        i++;
    }

    *index = *index - 1;
    *size_array= *size_array - 1;
}

void close_all_connections_by_select(int *array_sockfd, int size_array) {
    for (int i=1; i<size_array; i++) {
        close(array_sockfd[i]);
    }
}
