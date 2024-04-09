#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

pthread_t th_envoie,th_recept;

struct Args_Thread {
    int dS;
    bool* continu;
    char* msg;
};

void fin(int dS,char** msg) {
    shutdown(dS,2) ;
    free(msg[0]);
    free(msg[1]);
    free(msg);
    printf("Fin du programme");
}

bool lecture(int dS, char** msg){
    bool res = true;
    int taille;
    recv(dS,&taille, sizeof(int), 0);
    recv(dS, *msg,taille, 0);
    if((strcmp(*msg,"fin") == 0)){
        res = false;
    }
    puts(*msg);
    return res;
}

bool envoie(int dS, char** msg){
    bool res = true;
    printf("Ecrit un message : ");
    fgets(*msg,128,stdin);
    char *pos = strchr(*msg,'\n');
    *pos = '\0';
    if((strcmp(*msg,"fin") == 0)){
        res = false;
    }
    int taille = strlen(*msg)+1;
    send(dS, &taille, sizeof(int), 0);
    send(dS, *msg, taille, 0);
    return res;
}

void* reception(void* args_thread) {
    struct Args_Thread* args = (struct Args_Thread*)args_thread;
    while(args->continu){
        *(args->continu) = lecture(args->dS,&(args->msg));         
    }
    pthread_exit(0);
}

void* propagation(void* args_thread){
    struct Args_Thread* args = (struct Args_Thread*)args_thread;
    while(args->continu){
        *(args->continu) = envoie(args->dS,&(args->msg));
    }
    pthread_exit(0);
}

int main(int argc, char* argv[]){

    if (argc != 3) {
        printf("./client IP Port");
    }
    else{

        printf("Début programme\n");
        int dS = socket(PF_INET, SOCK_STREAM, 0);
        printf("Socket Créé\n");

        struct sockaddr_in aS;
        aS.sin_family = AF_INET;
        inet_pton(AF_INET,argv[1],&(aS.sin_addr)) ;
        aS.sin_port = htons(atoi(argv[2])) ;
        socklen_t lgA = sizeof(struct sockaddr_in) ;
        connect(dS, (struct sockaddr *) &aS, lgA) ;
        printf("Socket Connecté\n");

        bool* continu = (bool*)malloc(sizeof(bool));
        *continu = true;

        char* msg_envoie = malloc(128*sizeof(char));
        char* msg_lecture = malloc(128*sizeof(char));

        struct Args_Thread* args_recept;
        args_recept->dS = dS;
        args_recept->continu = continu;
        args_recept->msg = msg_lecture;

        struct Args_Thread* args_envoie;
        args_recept->dS = dS;
        args_recept->continu = continu;
        args_recept->msg = msg_envoie;

        void* arg_recept = (dS,&continu,msg_lecture);
        void* arg_envoie = (dS,&continu,msg_lecture);

        pthread_create(&th_recept, NULL, reception, args_recept);

        pthread_create(&th_envoie,NULL, propagation, args_envoie);

        pthread_join(th_recept,NULL);
        pthread_join(th_envoie,NULL);

        char** msg = (char**)malloc(2*sizeof(char*));
        msg[0] = msg_envoie;
        msg[1] = msg_lecture;
        fin(dS,msg);
    }
}