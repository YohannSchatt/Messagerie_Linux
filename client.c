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

void lecture(int dS, char** msg){
    bool res = true;
    int taille;
    recv(dS,&taille, sizeof(int), 0);
    recv(dS, *msg,taille, 0);
    puts(*msg);
}

void envoie(int dS, char** msg){
    printf("coucou\n");
    bool res = true;
    printf("Ecrit un message : ");
    fgets(*msg,128,stdin);
    char *pos = strchr(*msg,'\n');
    *pos = '\0';
    printf("%s",*msg);
    if(strcmp(*msg,"fin") == 0){
        res = false;
    }
    int taille = strlen(*msg)+1;
    send(dS, &taille, sizeof(int), 0);
    send(dS, *msg, taille, 0);
}

void* reception(void* args_thread1) {
    printf("je suis dans reception\n");
    char* msg = (char*)malloc(128*sizeof(char));
    struct Args_Thread args = *((struct Args_Thread*)args_thread1);
    bool* continu = args.continu;
    int dS = args.dS;
    printf("bool = %d\n",*continu);
    while(true){
        lecture(dS,&msg);         
    }
    pthread_exit(0);
}

void* propagation(void* args_thread2){
    printf("je suis dans propagation\n");
    char* msg = (char*)malloc(128*sizeof(char));
    struct Args_Thread args = *((struct Args_Thread*)args_thread2);
    //bool* continu = args.continu;
    int dS = args.dS;
    printf("%d\n",dS);
    //printf("bool = %d\n",*continu);
    while(true){
        envoie(dS,&msg);
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
        printf("%d\n",dS);
        struct sockaddr_in aS;
        aS.sin_family = AF_INET;
        inet_pton(AF_INET,argv[1],&(aS.sin_addr)) ;
        aS.sin_port = htons(atoi(argv[2])) ;
        socklen_t lgA = sizeof(struct sockaddr_in) ;
        connect(dS, (struct sockaddr *) &aS, lgA) ;
        printf("Socket Connecté\n");

        bool* continu = (bool*)malloc(sizeof(bool));
        *continu = true;

        char* msg_envoie = (char*)malloc(128*sizeof(char));
        char* msg_lecture = (char*)malloc(128*sizeof(char));


        struct Args_Thread args_recept;
        args_recept.dS = dS;
        args_recept.continu = continu;
        args_recept.msg = msg_lecture;
        printf("%d\n",args_recept.dS);

        struct Args_Thread args_envoie;
        args_envoie.dS = dS;
        args_envoie.continu = continu;
        args_envoie.msg = msg_envoie;
        printf("%d\n",args_envoie.dS);

        if (pthread_create(&th_recept, NULL, reception, (void*)&args_recept) == -1) {
            fprintf(stderr, "Erreur lors de la création du thread\n");
            return EXIT_FAILURE;
        }

        if (pthread_create(&th_envoie,NULL, propagation, (void*)&args_envoie) == -1) {
            fprintf(stderr, "Erreur lors de la création du thread\n");
            return EXIT_FAILURE;
        }

        pthread_join(th_recept,NULL);
        pthread_join(th_envoie,NULL);
        

        char **msg = (char**)malloc(2*sizeof(char*));
        msg[0] = args_envoie.msg;
        msg[1] = args_recept.msg;
        fin(dS,msg);
    }
}