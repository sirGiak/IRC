#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "headers/common.h"

#define SERVER_IP "127.0.0.1"


int start_tcp_connection(struct sockaddr_in *serv_addr) {
    int sockfd;
    // Crea il socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Configura l'indirizzo del server
    serv_addr->sin_family = AF_INET;
    serv_addr->sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr->sin_addr) <= 0) {
        perror("indirizzo non valido");
        exit(EXIT_FAILURE);
    }

    // Connette al server
    if (connect(sockfd, (struct sockaddr *)serv_addr, sizeof(*serv_addr)) < 0) {
        perror("connessione fallita");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void print_help(){
    printf("Possibili azioni: (tutti i parametri dentro delle parentesi sono opzionali\n");
    printf("Imposta nickname: nick <nickname>\n");
    printf("Unisciti ai canali: join <#channel1>{,<#channel2>} \n");
    printf("Spedisci messaggi: <receiver>{,<receiver>} <:testo che deve essere spedito>\n\t<receiver> = <#channel>"
           " o <nickname>\n");
    printf("GET o SET argomento del canale: topic <#channel> [:<topic>]\n");
    printf("Esci dai canali: part <#channel1>{,<#channel2>}\n");
    printf("Abbandona connessione: quit [:<quit message>]\n");
}

void analyze_response(char *buffer) {
    ;
}