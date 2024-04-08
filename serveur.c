#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

void fin(int dSC) {
    shutdown(dSC,2) ;
    printf("fermeture");
}

bool lecture(int dSC){
    bool res = true;
    char* msg = malloc(128*sizeof(char));
    recv(dSC, msg, (strlen(msg)+1)*sizeof(char), 0) ;
    if(msg == "fin"){
        res = false;
    }
    else {
        printf("%s\n",msg);
    }
    free(msg);
    return res;
}

bool envoie(int dSC){
    bool res = true;
    char* msg = malloc(128*sizeof(char));
    fgets(msg,128,stdin);
    if(msg == "fin"){
        res = false;
    }
    else {
        send(dSC, msg, (strlen(msg)+1)*sizeof(char) , 0) ;
    }
    free(msg);
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

    printf("r1\n");
    send(dSC1, &r1, sizeof(int), 0) ;

    printf("r2\n");
    send(dSC2, &r2, sizeof(int), 0) ;

    int* tabdSC = malloc(2*sizeof(int));
    tabdSC[0] = dSC1;
    tabdSC[1] = dSC2; 

    bool continu = true;
    int pos = 0;

    printf("%d\n",continu);

    while(continu){
        printf("je lis\n");
        lecture(tabdSC[pos]);
        printf("j'envoie\n");
        envoie(tabdSC[(pos+1)%2]);
        pos = (pos+1)%2;
    }
    fin(tabdSC[0]);
    fin(tabdSC[1]);
    printf("Fin du programme");
}