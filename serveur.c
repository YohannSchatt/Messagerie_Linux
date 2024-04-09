#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

void fin(int dSC,char** msg) {
    free(*msg);
    shutdown(dSC,2) ;
    printf("fermeture");
}

bool lecture(int dSC,char **msg){
    bool res = true;
    int taille;
    recv(dSC,&taille, sizeof(int), 0);
    recv(dSC, *msg, taille, 0);
    if(*msg == "fin\n\0"){
        res = false;
    }
    printf("%s\n",*msg);
    return res;
}

bool envoie(int dSC,char** msg){
    bool res = true;
    if(*msg == "fin\n\0"){
        res = false;
    }
    int taille = strlen(*msg)+1;
    send(dSC, &taille, sizeof(int), 0);
    send(dSC, *msg, taille , 0);
    return res;
}

int main(int argc, char *argv[]) {
  
    printf("Début programme\n");

    int dS = socket(PF_INET, SOCK_STREAM, 0);
    printf("Socket Créé\n");


    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY ;
    ad.sin_port = htons(atoi(argv[1])) ;
    bind(dS, (struct sockaddr*)&ad, sizeof(ad)) ;
    printf("Socket Nommé\n");
    listen(dS, 7);
    printf("Mode écoute\n");

    struct sockaddr_in aC ;
    socklen_t lg1 = sizeof(struct sockaddr_in) ;
    int dSC1 = accept(dS, (struct sockaddr*) &aC,&lg1) ;
    printf("Client 1 Connecté\n");

    struct sockaddr_in aB ;
    socklen_t lg2 = sizeof(struct sockaddr_in) ;
    int dSC2 = accept(dS, (struct sockaddr*) &aB,&lg2) ;
    printf("Client 2 Connecté\n");

    int r1 = 0;
    int r2 = 1;

    send(dSC1, &r1, sizeof(int), 0) ;

    send(dSC2, &r2, sizeof(int), 0) ;

    int* tabdSC = malloc(2*sizeof(int));
    tabdSC[0] = dSC1;
    tabdSC[1] = dSC2; 

    bool continu = true;
    int pos = 1;

    char* msg = malloc(128*sizeof(char));

    shutdown(dS,2); //on ferme le socket de demande de connection car inutile

    while(continu){
        continu = lecture(tabdSC[pos],&msg);
        continu = envoie(tabdSC[(pos+1)%2],&msg);
        pos = (pos+1)%2;
    }
    fin(tabdSC[0],&msg);
    fin(tabdSC[1],&msg);
    printf("Fin du programme");
}