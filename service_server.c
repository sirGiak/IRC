#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include "headers/service_server.h"
#include <sys/socket.h>
#include "headers/common.h"

users u = {0};
channels c = {0};
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;

void nick(int sockfd, char* nickname) {
    char buffer[BUFFER_SIZE-2]={0};
    if (strlen(nickname)>0) {
        if (strlen(nickname)>=SIZE_NICKNAME)
            nickname[SIZE_NICKNAME-1]='\0'; //riduco la lunghezza del nickname

        if (is_middle(nickname) && nickname[0]!='#') {
            pthread_mutex_lock(&mutex);

            user *p_u=get_user(sockfd);
            if (p_u!=NULL) {//se utente trovato
                if (get_user_by_nickname(nickname)!=NULL) {//se nickname esiste già
                    if (!strcmp(p_u->nickname, nickname)) //se l'utente ha già lo stesso nickname
                        sprintf(buffer, "433 %s :Nickname già in uso", nickname);
                    else
                        sprintf(buffer, "436 %s :Collisione di nickname", nickname);

                    send_response(sockfd, buffer);
                }else
                    strcpy(p_u->nickname, nickname); //imposto il nuovo nickname
            }else {//creo un nuovo utente
                //alloco memoria
                if (u.size==0) {
                    u.array=malloc(sizeof(user));
                    u.size++;
                }else {
                    u.size++;
                    u.array=realloc(u.array,u.size*sizeof(user));
                }
                p_u=&u.array[u.size-1];
                //setto i dati del nuovo utente
                p_u->sockfd=sockfd;
                strcpy(p_u->nickname, nickname);
                printf("Utente creato: %s\n", p_u->nickname);
            }

            pthread_mutex_unlock(&mutex);
        }else {
            sprintf(buffer, "432 %s :Nickname non valido", nickname);
            send_response(sockfd, buffer);
        }
    }else
        send_response(sockfd, "432 nick :Nickname non dato");
}

void join(int sockfd, char* channel_names) {
    bool fine=false;
    user *p_u=NULL;

    pthread_mutex_lock(&mutex);

    p_u=get_user(sockfd);
    if (p_u!=NULL) {
        do{
            char buffer[BUFFER_SIZE-2]={0};
            char *p_channel=channel_names;
            char *p_comma=strchr(channel_names,','); //puntamento alla prima virgola di channel_names

            if (p_comma==NULL) {
                fine=true;
            }else {
                *p_comma='\0'; //così p_channel si fermerà a p_comma
                channel_names=p_comma+1; //channel_names diventa tutto ciò che c'è dopo la prima virgola
            }
            if (strlen(p_channel)>1) {
                if (strlen(p_channel)>=SIZE_CHANNEL_NAME) // se il nome del canale supera la lunghezza massima possibile
                    p_channel[SIZE_CHANNEL_NAME-1]='\0'; //riduco la lunghezza
                if (p_channel[0]=='#') {
                    p_channel++; //in modo da saltare '#'
                    if (is_middle(p_channel)) {
                        channel *p_ch=get_channel(p_channel);
                        if (p_ch!=NULL) {// se è stato trovato il canale
                            join_in_a_channel(p_ch, p_u, sockfd); //mi unisco
                        }else {//creo canale e mi unisco
                            c.size++;
                            if (c.size==1) {
                                c.array=malloc(sizeof(channel));
                            }else {
                                c.array=realloc(c.array,c.size*sizeof(channel));
                            }
                            channel *new_ch=&c.array[c.size-1];
                            // Inizializzo nuovo canale
                            strcpy(new_ch->name, p_channel);
                            new_ch->fd_operator=p_u->sockfd; //set operatore
                            new_ch->topic[0]='\0';
                            new_ch->n_members=0;
                            new_ch->fd_members=NULL;
                            printf("Canale creato #%s - %s\n", p_channel, new_ch->name);

                            join_in_a_channel(new_ch, p_u, sockfd);
                        }
                    }else {
                        sprintf(buffer, "403 #%s :Nome canale non valido", p_channel);
                        send_response(sockfd, buffer);
                    }
                }else {
                    sprintf(buffer, "403 %s :Nome canale non valido", p_channel);
                    send_response(sockfd, buffer);
                }
            }else {
                sprintf(buffer, "403 %s :Nome canale non valido", p_channel);
                send_response(sockfd, buffer);
            }
        }while(!fine);
    }else {
        send_response(sockfd, "451 :Registrati tramite nick prima");
    }
    pthread_mutex_unlock(&mutex);
}

void part(int sockfd, char* channel_names) {
    char buffer[BUFFER_SIZE-2]={0};

    if (strlen(channel_names)>1) {//se stringa non vuota
        if (is_middle(channel_names)) {// se stringa valida
            do {
                bool uscito=false;
                char ch_name[SIZE_CHANNEL_NAME]={0};

                channel_names=get_channel_name(ch_name,channel_names);
                if (strlen(ch_name)>0 && is_middle(ch_name)) {//se è valido e non vuoto
                    pthread_mutex_lock(&mutex);

                    channel *p_ch=get_channel(ch_name);
                    if (p_ch!=NULL) {//se esiste
                        if (exit_the_channel(sockfd, p_ch)) //se è uscito dal canale
                            uscito=true;
                        else // utente non è membro
                            sprintf(buffer,"442 #%s :Non fai parte del canale", ch_name);
                    }else
                        sprintf(buffer, "403 #%s :Canale insesistente", ch_name);

                    pthread_mutex_unlock(&mutex);
                }else
                    sprintf(buffer, "403 #%s :Canale non valido", channel_names);

                if (!uscito)
                    send_response(sockfd, buffer);
            }while (channel_names!=NULL);// fin quando ci sono nomi da leggere
        }else {
            sprintf(buffer, "403 %s :Canali non validi", channel_names);
            send_response(sockfd, buffer);
        }
    }else
        send_response(sockfd, "461 part :Servono più parametri");
}

void topic(int sockfd, char* parameters) {
    char buffer[BUFFER_SIZE-2];

    if (strlen(parameters)>0) {//se params non è vuoto
        char *channel_name=parameters;
        char *topic=strchr(parameters, ' ');
        if (topic!=NULL) {
            topic[0]='\0';
            topic++;
        }
        if (channel_name[0]=='#' && is_middle(channel_name) && strlen(channel_name)>1) {//se <channel> è valido
            pthread_mutex_lock(&mutex);

            channel *p_ch=get_channel(++channel_name); //lo incremento per saltare '#'
            if (p_ch!=NULL) {// se il canale è stato trovato
                if (topic==NULL) { // se :<topic> è assente
                    if (is_member(p_ch, sockfd)) {
                        //invio l'argomento del canale
                        if (strlen(p_ch->topic)>0)
                            sprintf(buffer, "332 #%s :%s",p_ch->name,p_ch->topic);
                        else
                            sprintf(buffer, "331 #%s :Nessun argomento presente",p_ch->name);
                    }else {
                        sprintf(buffer, "442 #%s :Non sei membro",p_ch->name);
                    }
                } else {
                    if (is_trailing(topic)) {// se :<topic> è valido
                        topic++; //salto i ':'
                        if (p_ch->fd_operator==sockfd) {//se l'utente è l'operatore del canale
                            if (strlen(topic)>=SIZE_TOPIC) // se l'argomento supera la lunghezza massima possibile
                                topic[SIZE_TOPIC-1]='\0'; // riduco la lunghezza
                            strcpy(p_ch->topic, topic); //imposto l'argomento del canale
                            sprintf(buffer, "332 #%s :%s",p_ch->name,p_ch->topic);
                        }else
                            sprintf(buffer, "482 #%s :Non sei operatore",p_ch->name);
                    }else
                        sprintf(buffer, "421 topic #%s %s :Sintassi errata",channel_name, topic);
                }
            }else
                sprintf(buffer, "403 #%s :Non esiste",channel_name);

            pthread_mutex_unlock(&mutex);
        }else
            sprintf(buffer, "403 %s :Nome canale non valido",channel_name);

    }else
        strcpy(buffer, "461 topic :Servono più parametri\0");

    send_response(sockfd, buffer);

}

void quit(int sockfd) {
    bool trovato=false;

    pthread_mutex_lock(&mutex);

    for (int i=0; i<c.size; i++) {
        if (is_member(&c.array[i], sockfd)) {
            exit_the_channel(sockfd, &c.array[i]);
        }
    }
    for (int i=0; i<u.size; i++) {
        if (u.array[i].sockfd==sockfd) {
            trovato=true;
        }
        if (trovato && i<u.size-1) {
            u.array[i]=u.array[i+1];
        }
    }
    if (trovato) {
        u.size--;
        u.array=realloc(u.array,u.size*sizeof(user));
        printf("Utente %d eliminato\n", sockfd);
    }
    pthread_mutex_unlock(&mutex);

}

void privmsg(int sockfd, char* destinations, char* text) {
    char buffer[BUFFER_SIZE-2];
    bool fine=false;
    user *p_u;

    pthread_mutex_lock(&mutex);

    p_u=get_user(sockfd);
    if (is_trailing(text) && strlen(text)>1){//se testo non vuoto ed è valido
        text++;
        if (strlen(destinations)>0 && is_middle(destinations)) {//se destinations non vuota ed è valida
            if (p_u!=NULL) { //se utente registrato
                do {
                    //leggi una destinazione
                    char *dest=destinations;
                    destinations=strchr(destinations,',');
                    if (destinations==NULL)
                        fine=true;
                    else {
                        destinations[0]='\0';
                        destinations++;
                    }

                    if (dest[0]=='#') //se dest è un canale
                        send_to_channel(++dest, p_u, text);
                    else
                        send_to_client(dest, p_u, text);
                }while (!fine);
            }else {
                sprintf(buffer,"451 privmsg :Non sei registrato");
                send_response(sockfd, buffer);
            }
        }else {
            sprintf(buffer,"411 privmsg :Nessun destinatario dato");
            send_response(sockfd, buffer);
        }
    }else {
        sprintf(buffer,"412 privmsg :Testo da mandare assente");
        send_response(sockfd, buffer);
    }

    pthread_mutex_unlock(&mutex);
}
