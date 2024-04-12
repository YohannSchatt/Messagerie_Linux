#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

int NB_MAX_PERSONNE = 100;

struct Args_Thread {
    int id_tab;
    int dSC;
    int* dSC_autre;
};

void fin_connexion(int dSC,int **dSC_autre,int id) {
    *dSC_autre[id] = -1;
    shutdown(dSC,2) ;
    printf("fermeture\n");
}

bool lecture(int dSC,char **msg, int id){
    printf("je suis dans lecture\n");
    bool res = true;
    int taille;
    recv(dSC,&taille, sizeof(int), 0);
    printf("coucou");
    recv(dSC, *msg, taille, 0);
    printf("message de %d : %s\n",id,*msg);
    if((strcmp(*msg,"fin") == 0)){
        res = false;
    }
    printf("%s\n",*msg);
    return res;
}

void envoie(int dSC,char** msg){
    int taille = strlen(*msg)+1;
    send(dSC, &taille, sizeof(int), 0);
    send(dSC, *msg, taille , 0);
}

void* lecture_envoie(void* args_thread) {
    struct Args_Thread args = *((struct Args_Thread*)args_thread);
    char* msg = (char*)malloc(128*sizeof(char));
    bool continu = true;
    int id_tab = args.id_tab;
    int dSC = args.dSC;
    int* dSC_autre = args.dSC_autre;
    while (continu) {
        continu = lecture(dSC, &msg,id_tab);
        if (continu){
            for(int i = 0;i<NB_MAX_PERSONNE;i++) {
                if (dSC != dSC_autre[i] && dSC_autre[i] != -1) {
                    envoie(dSC_autre[i], &msg);  
                }
            }
        }
    }
    fin_connexion(args.dSC,&args.dSC_autre,args.id_tab);
}

int init_ouverture_connexion(int port) {
    
    int dS = socket(PF_INET, SOCK_STREAM, 0);
    printf("Socket Créé\n");

    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY ;
    ad.sin_port = htons(port) ;
    bind(dS, (struct sockaddr*)&ad, sizeof(ad)) ;
    printf("Socket Nommé\n");

    listen(dS, 7);
    printf("Mode écoute\n");

    return dS;
}

void init_connexion(int dS,int* tabdSC, pthread_t* thread) {

    printf("je suis dans init_connexion\n");

    int count = 0;

    for(int i = 0; i<NB_MAX_PERSONNE;i++) {
        tabdSC[i] = -1;
    }
    int i = 0;

    while(true){

        struct sockaddr_in aC ;
        socklen_t lg = sizeof(struct sockaddr_in) ;
        if (count < NB_MAX_PERSONNE){
            printf("j'attend l'acceptation\n");
            int dSC = accept(dS, (struct sockaddr*) &aC,&lg) ;
            printf("Client Connecté\n");

            while(tabdSC[i] != -1){
                i = (i + 1)%NB_MAX_PERSONNE;
            }
            tabdSC[i] = dSC;

            struct Args_Thread args;
            args.id_tab = i;
            args.dSC = dSC;
            args.dSC_autre = tabdSC;

            printf("je crée un thread avec l'id : %d\n",i);

            pthread_create(&thread[i], NULL, lecture_envoie,(void*)&args);
            count++;
        }
    }
}

int main(int argc, char *argv[]) {

    printf("Début programme\n");

    pthread_t* thread = (pthread_t*)malloc(NB_MAX_PERSONNE*sizeof(pthread_t));

    int dS = init_ouverture_connexion(atoi(argv[1]));

    char* msg = malloc(128*sizeof(char));

    int* tabdSC = (int*)malloc(NB_MAX_PERSONNE*sizeof(int));

    init_connexion(dS,tabdSC,thread);

    free(tabdSC);
    free(msg);
    shutdown(dS,2);
    printf("fin programme");
}