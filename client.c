#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

void fin(int dS,char** msg) {
    shutdown(dS,2) ;
    free(*msg);
    printf("Fin du programme");
}

bool lecture(int dS, char** msg){
    bool res = true;
    int taille;
    recv(dS,&taille, sizeof(int), 0);
    recv(dS, *msg,128+1, 0);
    if(*msg == "fin\0"){
        res = false;
    }
    printf("\n");
    printf("L'autre utilisateur dit : %s\n",*msg);
    return res;
}

bool envoie(int dS, char** msg){
    bool res = true;
    printf("Ecrit un message : ");
    fgets(*msg,128,stdin);
    char *pos = strchr(*msg,'\n');
    *pos = '\0';
    if(*msg == "fin\0"){
        res = false;
    }
    int taille = strlen(*msg)+1;
    send(dS, &taille, sizeof(int), 0);
    send(dS, *msg, taille, 0);
    return res;
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

        bool continu = true;
        int pos;

        recv(dS, &pos, sizeof(int), 0);

        char* msg = malloc(128*sizeof(char));

        while(continu){
            if (pos == 1) {
                continu = envoie(dS,&msg);
            }
            else {
                continu = lecture(dS,&msg); 
            }
            pos = (pos+1)%2;
        }
        fin(dS,&msg);
    }
}