#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include "headers/common.h"
#include "headers/client.h"

int sockfd;
bool quit;
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;

int main() {
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE]={0};
    pthread_t thread;

    sockfd = start_tcp_connection(&serv_addr);

    if (sockfd>0) {
        quit=false;

        printf("Connessione al server attiva. Invia messaggi (scrivi 'help' per la guida):\n");

        pthread_create(&thread, NULL, read_IRC_replies, NULL);

        pthread_mutex_lock(&mutex);
        while (!quit) {
            pthread_mutex_unlock(&mutex);
            fgets(buffer, BUFFER_SIZE-2, stdin);

            if(strlen(buffer) > 0) {
                if(strcmp(buffer, "help\n") == 0)
                    print_help();
                else{
                    buffer[strlen(buffer)-1]='\r';
                    buffer[strlen(buffer)]='\n';

                    send(sockfd, buffer, strlen(buffer), 0);

                    if (strncmp(buffer, "quit", 4) == 0 || strcmp(buffer, "server_off\r\n") == 0) {
                        printf("Uscita...\n");
                        pthread_mutex_lock(&mutex);
                        quit=true;
                        pthread_mutex_lock(&mutex);
                    }
                }
            }
            memset(buffer, 0, BUFFER_SIZE);
        }
        pthread_mutex_unlock(&mutex);
        close(sockfd);
    }
    return 0;
}

void *read_IRC_replies() {
    time_t t;
    char *c_time=NULL;
    char buffer[BUFFER_SIZE]= {0};

    pthread_mutex_lock(&mutex);
    while (!quit) {
        pthread_mutex_unlock(&mutex);
        int n = read(sockfd, buffer, BUFFER_SIZE);

        if (n > 0) {
            buffer[n-2] = '\0';
            if(strcmp(buffer, "quit") == 0) {
                printf("Uscita...\n");
                pthread_mutex_lock(&mutex);
                quit=true;
                pthread_mutex_unlock(&mutex);
            }else
                analyze_response(buffer);

            t=time(NULL);
            c_time=ctime(&t);
            c_time[strlen(c_time)-1]='\0';
            printf("%s: %s\n", c_time, buffer);
        }
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}
