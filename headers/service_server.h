//
// Created by giacomo on 26/08/25.
//

#ifndef SERVICE_SERVER_H
#define SERVICE_SERVER_H

#define SIZE_CHANNEL_NAME 201
#define SIZE_NICKNAME 10
#define SIZE_TOPIC 301
#define MAX_CHANNELS 400
#define MAX_USERS 1000
#define MAX_JOINED_CHANNELS 20
#define MAX_MEMBERS 100

typedef struct {
    int sockfd;
    char nickname[SIZE_NICKNAME];
}user;

typedef struct {
    user *array;
    int size;
}users;

typedef struct {
    char name[SIZE_CHANNEL_NAME];
    int fd_operator;
    int *fd_members;
    int n_members;
    char topic[SIZE_TOPIC];
}channel;

typedef struct {
    channel *array;
    int size;
}channels;

//utils_server.c
void send_response(int sockfd, char response[]);

//utils_service_server.c
bool is_middle(char *parameter);
bool is_trailing(char * parameter);
user *get_user(int sockfd);
user *get_user_by_nickname(char *nickname);
channel *get_channel(char *c_name);
void join_in_a_channel(channel *p_ch, user *p_u, int sockfd);
bool is_member(channel *p_ch, int sockfd);
char *get_channel_name(char *dest, char *source);
bool exit_the_channel(int sockfd, channel *p_ch);
void send_to_client(char *nickname, user *p_u, char *text);
void send_to_channel(char *ch_name, user *p_u, char *text);

#endif //SERVICE_SERVER_H
