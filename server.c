#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#define BACKLOG 10
#define MAXDATALEN 1024 
#define MAXUSER 10     
#define MAXGROUP 5    
#define PORT 1234     

typedef struct{
    char username[10];
    int port;
} User;

void *server(void *arg);                                           
void insert_list(int port, char *username, User *list, int *tail); 
int search_list(int port, User *list, int tail);
void delete_list(int port, User *list, int *tail);
void delete_all(User *list, int *tail);
void display_list(const User *list, int tail);                                             
int next_space(char *str);

char username[10];
User users[MAXUSER] = {0};
int user_tail = 0;
User groups[MAXGROUP][MAXUSER] = {0};
int group_tail[MAXUSER] = {0};
char buffer[MAXDATALEN];

int main(int argc, char *argv[]){
    int sockfd, new_fd;
    int portnum;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    int cli_size, z;
    pthread_t thr;
    int flag = 1;

    printf("start\n");
    if (argc == 2)
        portnum = atoi(argv[1]);
    else
        portnum = PORT;

    //set info of server 
    server_addr.sin_family = AF_INET;               
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_addr.sin_port = htons(portnum);

    //creating socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1){
        printf("setsockopt error"); 
        exit(1);
    }
    //binding socket
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1){
        printf("binding failed\n"); 
        exit(1);
    }

    listen(sockfd, BACKLOG);
    printf("waiting\n");
    // Accepting new clients
    while (1){
        cli_size = sizeof(struct sockaddr_in);                               
        new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &cli_size); 

        /* getting username */
        bzero(username, 10);
        if (recv(new_fd, username, sizeof(username), 0) > 0);
        username[strlen(username) - 1] = ' ';
        printf("%s joind\n",username);
        insert_list(new_fd, username, users, &user_tail); 
        User args;
        args.port = new_fd;
        strcpy(args.username, username);
        pthread_create(&thr, NULL, server, (void *)&args); 
        pthread_detach(thr);
    }
    delete_all(users, &user_tail);
    close(sockfd);
}
//server handler
void *server(void *arguments){
    User *args = arguments;
    char buffer[MAXDATALEN], uname[10]; 
    char *strp;
    char *msg = (char *)malloc(MAXDATALEN);
    int my_port, x, y;
    int msglen;
    my_port = args->port;
    strcpy(uname, args->username);

    // Handling messages
    while (1){
        bzero(buffer, 256);
        y = recv(my_port, buffer, MAXDATALEN, 0);

        /* Client quits */
        if (!y || strncmp(buffer, "/quit", 5) == 0){
            printf("%s left chat\n", uname);

            delete_list(my_port, users, &user_tail);
            for (int i = 0; i < MAXGROUP; i++){
                delete_list(my_port, groups[i], &group_tail[i]);
            }

            display_list(users, user_tail);

            close(my_port);
            free(msg);

            break;
        }
        else if (strncmp(buffer, "/join", 5) == 0){
            char *group_id_str = malloc(sizeof(MAXDATALEN));
            strcpy(group_id_str, buffer + 6);
            int group_id = atoi(group_id_str);
            printf("%s joined group number %d\n", uname, group_id);

            insert_list(my_port, uname, groups[group_id], &group_tail[group_id]);
        }
        else if (strncmp(buffer, "/leave", 6) == 0){
            char *group_id_str = malloc(sizeof(MAXDATALEN));
            strcpy(group_id_str, buffer + 7);
            int group_id = atoi(group_id_str);
            printf("%s left group number %d\n",uname, group_id);

            delete_list(my_port, groups[group_id], &group_tail[group_id]);
        }
        else if (strncmp(buffer, "/send", 5) == 0){
            int space_pos = next_space(buffer + 6);
            char *group_id_str = malloc(sizeof(MAXDATALEN));
            strncpy(group_id_str, buffer + 6, space_pos);
            int group_id = atoi(group_id_str);

            if (search_list(my_port, groups[group_id], group_tail[group_id]) == -1) {
                continue;
            }

            printf("%s %s\n", uname, buffer);
            strcpy(msg, uname);
            x = strlen(msg);
            strp = msg;
            strp += x;
            strcat(strp, buffer + 7 + space_pos);
            msglen = strlen(msg);

            for (int i = 0; i < group_tail[group_id]; i++)
            {
                if (groups[group_id][i].port != my_port)
                    send(groups[group_id][i].port, msg, msglen, 0);
            }

            bzero(msg, MAXDATALEN);
        }
        display_list(users, user_tail);
    }
    return 0;
}

void insert_list(int port, char *username, User *list, int *tail){
    if (search_list(port, list, *tail) != -1){
        return;
    }
    User *temp;
    temp = malloc(sizeof(User));
    if (temp == NULL){
        printf("Out of space!");
    }
    temp->port = port;
    strcpy(temp->username, username);
    list[(*tail)++] = *temp;
}

int search_list(int port, User *list, int tail){
    for (int i = 0; i < tail; i++){
        if (list[i].port == port)
            return i;
    }
    return -1;
}

void delete_list(int port, User *list, int *tail){
    int ptr = search_list(port, list, *tail);
    if (ptr == -1){
        return;
    }

    for (int i = ptr; i < *tail - 1; i++){
        list[i] = list[i + 1];
    }
    (*tail)--;
}

void display_list(const User *list, int tail){
    printf("Current online users:\n");
    if (tail == 0){
        printf("No one is online\n");
        return;
    }

    for (int i = 0; i < tail; i++){
        printf("%s\t",list[i].username);
    }
    printf("\n");
}

void delete_all(User *list, int *tail){
    *tail = 0;
}

int next_space(char *str){
    int i = 0;
    while (str[i] != '\0'){
        if (str[i] == ' '){
            return i;
        }
        i++;
    }
    return -1;
}