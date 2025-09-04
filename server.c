#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "headers/common.h"
#include "headers/server.h"

pthread_cond_t cond=PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock= PTHREAD_MUTEX_INITIALIZER;
int contatore_threads; // indica il numero di thread in esecuzione

void *start_IRC_protocol(void *arg) {
    message *m = (message *)arg;
    char *parameters=strchr(m->buffer, ' ');

    printf("FD: %d. Messaggio ricevuto: %s\n", m->sockfd, m->buffer);
    if (parameters!=NULL) {
        parameters[0]='\0';
        parameters++;
        if (parameters[strlen(parameters)-2]=='\r' && parameters[strlen(parameters)-1]=='\n') {
            parameters[strlen(parameters)-2]='\0';
            if (!strcmp(m->buffer, "nick"))
                nick(m->sockfd, parameters);
            else if (!strcmp(m->buffer, "join"))
                join(m->sockfd, parameters);
            else if (!strcmp(m->buffer, "part"))
                part(m->sockfd, parameters);
            else if (!strcmp(m->buffer, "topic"))
                topic(m->sockfd, parameters);
            else if (!strcmp(m->buffer, "quit"))
                quit(m->sockfd);
            else{
                privmsg(m->sockfd, m->buffer, parameters); //m->buffer conterrà le destinazioni e params il testo.
            }
        }else {
            send_response(m->sockfd, "421 Comando sconosciuto");
        }
    }else {
        if (!strncmp(m->buffer, "quit",4))
            quit(m->sockfd);
        else
            send_response(m->sockfd, "421 Comando sconosciuto");
    }
    free(m); //deallocazione del messaggio dall'heap

    pthread_mutex_lock(&lock);
    contatore_threads--;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&lock);

    return NULL;
}

int main() {
    int array_sockfd[MAX_SOCKET]; //verrà shiftato verso sinistra nel momento che viene chiuso un socket. Evitando così di lasciare spazi vuoti e dunque di riempirlo.
    int size_array=0, max_sockfd=0; // size_array indica il numero di socket effettivi utile per la gestione di essi, e max_sockfd il file descriptor più alto utile per la select()
    struct sockaddr_in serv_addr, cli_addr; //contengono rispettivamente indirizzo IP e porta di ascolto del server e del client, utili per la comunicazione
    socklen_t clilen = sizeof(cli_addr);
    pthread_t thread; // utilizzato alle chiamate di pthread_create()
    fd_set read_fds; // insieme di file descriptor che il kernel deve monitorare
    struct timeval tv; // usato per stabilire l'intervallo di tempo massimo, che il processo deve stare in stato di attesa dopo la select()
    bool turn_off=false; // server acceso/spento

    array_sockfd[0] = init_socket_TCP(&serv_addr, MAX_NUM, PORT);
    size_array++;
    max_sockfd = array_sockfd[0];

    set_nonblocking(array_sockfd[0]);

    printf("Server multithread in ascolto su porta %d...\n", PORT);
    while (!turn_off) {
        init_fd_set(&read_fds, array_sockfd, size_array);
        // set timeout
        tv.tv_sec= 15;
        tv.tv_usec = 0;
        int ready_sockfd = select(max_sockfd+1, &read_fds, NULL,NULL, &tv);
        if (ready_sockfd == -1) {
            perror("select failed");
            exit(EXIT_FAILURE);
        }else {
            //ACCEPTING SOOCKET
            if (FD_ISSET(array_sockfd[0], &read_fds) && size_array < MAX_SOCKET) {
                int new_sockfd=accept_new_connection_by_select(array_sockfd, &size_array, &cli_addr, &clilen);
                //se il nuovo fd è maggiore di max_sockfd, questultimo viene aggiornato
                if (new_sockfd > max_sockfd)
                    max_sockfd = new_sockfd;
            }
            //READING SOCKETS
            if (size_array>1) {
                int i=1, j=0; // i serve a spazzolare tutti i fd, j tiene la conta degli fd letti
                while (i<size_array && j<ready_sockfd && !turn_off) {
                    if (FD_ISSET(array_sockfd[i], &read_fds)) {
                        message *m=malloc(sizeof(message)); //uso malloc perchè deve essere passato m al thread, altrimenti si sovrascriverebbe

                        int n_bytes = read(array_sockfd[i], m->buffer, BUFFER_SIZE-1);
                        if (n_bytes >0) {
                            m->buffer[n_bytes]='\0';
                            if((strcmp(m->buffer, "server_off\r\n") == 0)) {
                                close_all_connections_by_select(array_sockfd, size_array);
                                turn_off=true;
                                free(m);

                            }else {
                                m->sockfd = array_sockfd[i];
                                // attesa se il numero di threads in esecuzione è uguale a MAX_NUM
                                pthread_mutex_lock(&lock);
                                contatore_threads++;
                                while(contatore_threads>=MAX_NUM) {
                                    pthread_cond_wait(&cond, &lock);
                                }
                                pthread_mutex_unlock(&lock);

                                if (!strncmp(m->buffer, "quit",4))
                                    close_connection_by_select(array_sockfd, &size_array, &i);
                                
                                //Creazione thread!
                                pthread_create(&thread, NULL, start_IRC_protocol, m);
                            }
                        }
                        j++;
                    }
                    i++;
                }
            }
        }
    }


    //attesa terminazione tutti i threads
    pthread_mutex_lock(&lock);
    while (contatore_threads>0) {
        pthread_cond_wait(&cond, &lock);
    }
    pthread_mutex_unlock(&lock);

    close(array_sockfd[0]);
    printf("Server off.\n");
    return 0;
}