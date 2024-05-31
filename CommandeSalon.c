#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "communication_serveur.c"
#include "typeSalon.c"

char* concatAllTab(char** tab,int taille){
    int taillemsg;
    for(int i = 0; i < taille ;i++){
        taillemsg = strlen(tab[i])+1; // +1 pour \n
    }
    char** msg = malloc(sizeof(char*)*(taillemsg+1));
    int pos = 0;
    for(int i = 0; i < taille; i++){
        for(int j = 0; i < strlen(tab[i]); j++){
            msg[pos] = tab[i][j];
            pos++;
        }
        msg[pos] = '\n';
        pos++;
    }
    msg[pos] = '\0';
    return msg;
}

void join(int client,char* name){
    int id_salon = getIdSalon(name);
    if (id_salon >= 0){
        tabdSC[client].id_salon =  id_salon;
        AppendUserSalon(id_salon,client);
        envoie(tabdSC,strcat("vous avez rejoint le salon : ",name));
    }
    else {
        envoie(tabdSC[client].dSC,"Ce salon n'existe pas");
    }
}

void create(int client, char* name){

}

void delete(int client, char* name){

}

void getSalon(int client){

}

void connected(int client,char* name){
    int id_salon = getIdSalon(name);
    int nb = countNbClientSalon(id_salon);
    char** nameClient = malloc(sizeof(char*)*nb);
    char *str;
    str = malloc(12 * sizeof(char));
    char* msg = concatAllTab(getSalonUserPseudo(id_salon,&nb),nb); //recupère les pseudos puis les concatènes dans un messages
    envoie(tabdSC[client].dSC,strcat("salon : ",name));
    envoie(tabdSC[client].dSC,strcat("Nombre de personne connecté dans le salon : ",sprintf(str, "%d", nb)));
    envoie(tabdSC[client].dSC,msg);
}