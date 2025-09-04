//
// Created by giacomo on 29/08/25.
//
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include "headers/service_server.h"
#include <sys/socket.h>
#include "headers/common.h"

extern users u;
extern channels c;
extern pthread_mutex_t mutex;

//controlla se è valido un parametro centrale, es: nickname o un canale
bool is_middle(char *parameter) {
    if (parameter[0]!=':' &&  strchr(parameter, ' ')==NULL && strchr(parameter, '\r')==NULL && strchr(parameter, '\n') == NULL)

        return true;
    else
        return false;
}

//controlla se è valido un parametro finale, es: testo da inviare ad un client
bool is_trailing(char * parameter) {
    if (parameter[0]==':' && strchr(parameter, '\r')==NULL && strchr(parameter, '\n') == NULL)
        return true;
    else
        return false;
}

//torna un puntatore ad un utente facente parte di u
user *get_user(int sockfd) {
    bool trovato=false;
    int i=0;
    while (!trovato && i<u.size) {
        if (sockfd==u.array[i].sockfd) {
            trovato=true;
        }else {
            i++;
        }
    }
    return trovato?&u.array[i]:NULL;
}

//torna un puntatore ad un utente facente parte di u
user *get_user_by_nickname(char *nickname) {
    bool trovato=false;
    int i=0;
    while (!trovato && i<u.size) {
        if (!strcmp(nickname,u.array[i].nickname)) {
            trovato=true;
        }else {
            i++;
        }
    }
    return trovato?&u.array[i]:NULL;
}

//torna un puntatore ad un canale facente parte di c
channel *get_channel(char *c_name) {
    bool trovato=false;
    int i=0;
    while (!trovato && i<c.size) {
        if (!strcmp(c_name,c.array[i].name)) {
            trovato=true;
        }else {
            i++;
        }
    }
    return trovato?&c.array[i]:NULL;
}

//aggiunge p_u->sockfd in p_ch->fd_members
void join_in_a_channel(channel *p_ch, user *p_u, int sockfd) {
    char buffer[BUFFER_SIZE-2] ={0}; //messaggio di risposta
    if (p_ch->n_members<MAX_MEMBERS) {
        int i=0;
        bool fa_gia_parte=false;
        sprintf(buffer, "353 #%s :", p_ch->name);
        while (i<p_ch->n_members) {
            user *tmp_u=get_user(p_ch->fd_members[i]);
            strcat(buffer, tmp_u->nickname);
            strcat(buffer, ",");

            if (p_ch->fd_members[i]==sockfd) {
                fa_gia_parte=true;
            }
            i++;
        }
        if (fa_gia_parte) {
            memset(buffer, 0, BUFFER_SIZE-2);
            sprintf(buffer, "403 #%s :Fai già parte del canale", p_ch->name);
            send_response(sockfd, buffer);
        }else {// aggiungo l'utente al canale
            p_ch->n_members++;
            if (p_ch->n_members==1) {
                p_ch->fd_members=malloc(sizeof(int));
            }else {
                p_ch->fd_members=realloc(p_ch->fd_members,p_ch->n_members*sizeof(int));
            }

            p_ch->fd_members[p_ch->n_members-1]=p_u->sockfd; //utente appena unito
            // printf("NIck member %d : %s\n",p_ch->n_members-1, p_ch->members[p_ch->n_members-1]->nickname);

            strcat(buffer, p_u->nickname);
            send_response(sockfd, buffer); //353 lista degli utenti del gruppo
            memset(buffer, 0, BUFFER_SIZE-2);
            if (strlen(p_ch->topic)>0) {
                sprintf(buffer, "332 #%s :%s",p_ch->name,p_ch->topic);
            }else {
                sprintf(buffer, "331 #%s :Nessun argomento presente",p_ch->name);
            }
            send_response(sockfd, buffer); //331 o 332; Topic del gruppo
        }
    }else {
        sprintf(buffer, "471 #%s :Canale pieno", p_ch->name);
        send_response(sockfd, "471");
    }

}

//controlo se sockfd è un elemento di p_ch->fd_members
bool is_member(channel *p_ch, int sockfd) {
    bool trovato=false;
    int i=0;
    while (!trovato && i<p_ch->n_members) {
        if (p_ch->fd_members[i]==sockfd)
            trovato=true;
        else
            i++;
    }
    return trovato;
}
// imposta a dest il nome di un canale e restituisce la posizione da cui continuare su source.
// Se ritorna NULL significa che source non ha ulteriori caratteri.
char *get_channel_name(char *dest, char *source) {
    char *p_start=strchr(source,'#');
    p_start++;
    char *p_end=strchr(p_start,' ');
    if (p_end==NULL)
        p_end=strchr(p_start,',');
    if (p_end!=NULL) {
        int n_bytes=(int)(p_end-p_start);
        strncpy(dest, p_start, n_bytes);
        return ++p_end;
    }else {
        strcpy(dest, p_start);
        return NULL;
    }
}

//rimuove sockfd da p_ch->fd_members
//se il canale diventa vuoto lo elimina
bool exit_the_channel(int sockfd, channel *p_ch) {
    bool trovato=false;
    int i=0;
    while (i<p_ch->n_members) {
        if (p_ch->fd_members[i]==sockfd) {
            trovato=true;
        }
        if (trovato && i<p_ch->n_members-1) {
            p_ch->fd_members[i]=p_ch->fd_members[i+1];
        }
        i++;
    }
    if (trovato) {
        //diminuisco di 1 la dimensione di fd_members
        p_ch->n_members--;

        if (p_ch->n_members>0) {
            p_ch->fd_members=realloc(p_ch->fd_members,p_ch->n_members*sizeof(int));
            if (p_ch->fd_operator==sockfd) {
                p_ch->fd_operator=-1;
            }
        }else {//canale vuoto -> lo elimino
            bool canale_trovato=false;
            free(p_ch->fd_members);
            i=0;
            while (i<c.size) {
                if (&c.array[i]==p_ch) {
                    canale_trovato=true;
                }
                if (canale_trovato && i<c.size-1) {
                    c.array[i]=c.array[i+1];
                }
                i++;
            }
            if (canale_trovato) {
                c.size--;
                c.array=realloc(c.array,c.size*sizeof(channel));
            }

        }
    }

    return trovato;
}

//invia text ai membri di ch_name
void send_to_channel(char *ch_name, user *p_u, char *text) {
    char buffer[BUFFER_SIZE-2]={0};
    channel *p_ch=get_channel(ch_name);
    if (p_ch!=NULL) {
        if (is_member(p_ch, p_u->sockfd)) {
            for (int i=0; i<p_ch->n_members; i++) {
                if (p_ch->fd_members[i]!=p_u->sockfd) { // controllo per non inviarsi il messaggio a se stesso
                    sprintf(buffer, "301 #%s - %s :%s", ch_name, p_u->nickname, text);
                    send_response(p_ch->fd_members[i], buffer);
                }
            }
        }else {
            sprintf(buffer, "442 %s :Non fai parte del canale", p_ch->name);
            send_response(p_u->sockfd, buffer);
        }
    }else {
        sprintf(buffer, "401 %s :Canale inesistente", ch_name);
        send_response(p_u->sockfd, buffer);
    }
}

//invia text a nickname
void send_to_client(char *nickname, user *p_u, char *text) {
    char buffer[BUFFER_SIZE-2];
    user *p_u_dest= get_user_by_nickname(nickname);
    if (p_u_dest!=NULL) {
        sprintf(buffer, "301 %s :%s", p_u->nickname, text);
        send_response(p_u_dest->sockfd, buffer);
    }else {
        sprintf(buffer, "401 %s :Nickname inesistente", nickname);
        send_response(p_u->sockfd, buffer);
    }
}