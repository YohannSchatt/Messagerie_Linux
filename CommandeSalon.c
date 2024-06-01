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
    char* msg = malloc(sizeof(char)*(taillemsg+1));
    int pos = 0;
    for(int i = 0; i < taille; i++){
        for(int j = 0; j < strlen(tab[i]); j++){
            msg[pos] = tab[i][j];
            pos++;
        }
        msg[pos] = '\n';
        pos++;
    }
    msg[pos] = '\0';
    return msg;
}

char* concat(char* c1, char* c2){
    char* res = malloc(sizeof(char)*(strlen(c1)+strlen(c2)+1));
    strcat(res,c1);
    strcat(res,c2);
    res[strlen(c1)+strlen(c2)] = '\0';
    return res;
}

void join(int client,char* name){
    int id_salon = getIdSalon(name);
    if (id_salon >= 0){
        pthread_mutex_lock(&M1);
        printf("id_salon : %d\n",tabdSC[client].id_salon);
        RemoveUserSalon(tabdSC[client].id_salon,client);
        tabdSC[client].id_salon = id_salon;
        AppendUserSalon(id_salon,client);
        pthread_mutex_unlock(&M1);
        envoie(tabdSC[client].dSC,concat("vous avez rejoint le salon : ",name));
    }
    else {
        envoie(tabdSC[client].dSC,"Ce salon n'existe pas ou il n'y plus de place");
    }
}

void create(int client, char* name){
    if(createSalon(name,client)){
        envoie(tabdSC[client].dSC,"Le salon est créé");
    }
    else {
        envoie(tabdSC[client].dSC,"erreur lors de la création du salon");
    }
}

void delete(int client, char* name){
    int id_salon = getIdSalon(name);
    pthread_mutex_lock(&M1);
    deleteSalon(id_salon);
    pthread_mutex_unlock(&M1);
    envoie(tabdSC[client].dSC,"Le salon est supprimé");
}

void getSalon(int client){
    pthread_mutex_lock(&M1); //bloque l'accès au tableau
    int nb = countNbSalon();
    char** tab = malloc(sizeof(char*)*nb);
    tab = getAllSalonName(nb);
    char* msg = concatAllTab(tab,nb); //recupère les pseudos puis les concatènes dans un messages
    char* str = malloc(12 * sizeof(char));
    sprintf(str, "%d", nb);
    envoie(tabdSC[client].dSC,concat("Nombre de salon : ",str));
    envoie(tabdSC[client].dSC,concat("salon :\n",msg));
    pthread_mutex_unlock(&M1); //on redonne l'accès au tableau
    free(tab);
    free(str);
}

void connected(int client,char* name){
    int id_salon = getIdSalon(name);
    pthread_mutex_lock(&M1); //bloque l'accès au tableau
    int nb = countNbClientSalon(id_salon);
    char** tab = malloc(sizeof(char*)*nb);
    tab = getSalonUserPseudo(id_salon,nb);
    char* msg = concatAllTab(tab,nb); //recupère les pseudos puis les concatènes dans un messages
    envoie(tabdSC[client].dSC,concat("salon : ",name));
    char* str = malloc(12 * sizeof(char));
    sprintf(str, "%d", nb);
    envoie(tabdSC[client].dSC,concat("Nombre de personne connecté dans le salon : ",str));
    envoie(tabdSC[client].dSC,concat("personne connecté dans ce salon :\n",msg));
    pthread_mutex_unlock(&M1); //on redonne l'accès au tableau
    free(tab);
    free(str);
}