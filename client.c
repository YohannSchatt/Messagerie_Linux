#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void fin(int dS) {
    shutdown(dS,2) ;
    printf("Fin du programme");
}

bool lecture(int dS){
    bool res = true;
    char* msg = malloc(128*sizeof(char*));
    recv(dS, msg, (strlen(msg)+1)*sizeof(char*), 0) ;
    if(msg == "fin"){
        res = false;
    }
    else {
        printf("%s\n",msg);
    }
    free(msg);
    return false
}

void envoie(int dS){
    char* msg = malloc(128*sizeof(char*));
    fgets(msg,128,stdin);
    send(dS, msg, (strlen(m)+1)*sizeof(char*) , 0) ;
    free(msg);
}

int main(int argc, char* argv[]){

    if (argc != 3) {
        printf("./client IP Port")
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

        while(continu){
            envoie(dS);
            lecture(dS); 
        }

        fin(dS);
    }
}