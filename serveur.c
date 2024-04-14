#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

#define NB_MAX_PERSONNE 100
int NB_PERSONNE_ACTUELLE = 0;

pthread_mutex_t M1 = PTHREAD_MUTEX_INITIALIZER;

int tabdSC[NB_MAX_PERSONNE];

struct Args_Thread {
    int id;
    int dSC;
};

void fin_connexion(int dSC,int id) {
    pthread_mutex_lock(&M1);
    tabdSC[id] = -1;
    NB_PERSONNE_ACTUELLE--;
    pthread_mutex_unlock(&M1);
    shutdown(dSC,2);
    printf("fermeture\n");
}

bool lecture(int dSC,char **msg){
    bool res = true;
    int taille;
    recv(dSC,&taille, sizeof(int), 0);
    recv(dSC, *msg, taille, 0);
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
    int id = args.id;
    int dSC = args.dSC;
    while (continu) {
        continu = lecture(dSC, &msg);
        if (continu){
            for(int i = 0;i<NB_MAX_PERSONNE;i++) {
                pthread_mutex_lock(&M1);
                if (tabdSC[i] != -1 && dSC != tabdSC[i]) {
                    envoie(tabdSC[i], &msg);  
                }
                pthread_mutex_unlock(&M1);
            }
        }
    }
    fin_connexion(dSC,id);
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

    int count = 0;

    for(int i = 0; i<NB_MAX_PERSONNE;i++) {
        tabdSC[i] = -1;
    }
    int i = 0;

    struct sockaddr_in aC ;
    socklen_t lg = sizeof(struct sockaddr_in) ;

    while(true){

        int dSC = accept(dS, (struct sockaddr*) &aC,&lg) ;
        
        if (count < NB_MAX_PERSONNE-1){
            printf("Client Connecté\n");

            pthread_mutex_lock(&M1);
            while(tabdSC[i] != -1){
                i = (i + 1)%NB_MAX_PERSONNE;
            }
            tabdSC[i] = dSC;
            NB_PERSONNE_ACTUELLE++;
            printf("dSC%d : %d\n",i,dSC);

            pthread_mutex_unlock(&M1);

            struct Args_Thread args;
            args.id = i;
            args.dSC = dSC;

            pthread_create(&thread[i], NULL, lecture_envoie,(void*)&args);
        }
        else {
            shutdown(dSC,2);
        }
    }
}

int main(int argc, char *argv[]) {

    printf("Début programme\n");

    pthread_t* thread = (pthread_t*)malloc(NB_MAX_PERSONNE*sizeof(pthread_t));

    int dS = init_ouverture_connexion(atoi(argv[1]));

    init_connexion(dS,tabdSC,thread);
    shutdown(dS,2);
    printf("fin programme");
}