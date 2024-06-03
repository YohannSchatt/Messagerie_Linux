#include "serveur.h"
#include <stdlib.h>
#include <stdbool.h>

#define NB_MAX_SALON 20 //limite max du nombre de salon 


struct salon* tabSalon[NB_MAX_SALON]; //tableau des salons

/**
 * @brief Initialise les salons
*/
void initSalon(){
    for(int i; i<NB_MAX_SALON; i++){
        tabSalon[i] = NULL;
    }
}


/**
 * @brief Renvoie le nom du salon
 * @param id du salon dans le tableau salon
 * @return nom du salon
*/
char* getSalonName(int id){
    return tabSalon[id]->nom;
}

/**
 * @brief Renvoie l'id du propriétaire du salon
 * @param id du salon dans la tableau salon
 * @return l'id du propréitaire
*/
int getIdProp(int id){
    return tabSalon[id]->id;
}

/**
 * @brief Renvoie le le nombre max de personne possible dans le salon
 * @param id du salon dans la tableau salon
 * @return Nombre max
*/
int getNbMax(int id){
    return tabSalon[id]->NB_MAX;
}

/**
 * @brief setteur qui modifie la valeur du NBMAX
 * @param id du salon dans la tableau salon
 * @param nb la nouvelle valeur
*/
bool setSalonNbMax(int id,int nb){
    if (nb<=NB_MAX_PERSONNE_SALON && nb > 1){
        tabSalon[id]->NB_MAX = nb;
        return true;
    }
    return false;
}

/**
 * @brief setteur qui permet de modifier le nom du salon
 * @param id du salon
 * @param nom nouveau nom du salon
*/
void setSalonName(int id,char* nom){
    tabSalon[id]->nom = nom;
}

/**
 * @brief setteur qui change le propriétaire du salon
 * @param id du propriétaire
 * @param new id du nouveau propriétaire
*/
void setSalonProp(int id, int new){
    tabSalon[id]->id = new;
}

/**
 * @brief Fonction qui permet d'obtenir l'id d'un salon
 * @param name le nom du salon
 * @return l'id du salon
*/
int getIdSalon(char* name){
    for(int i = 0;i<NB_MAX_SALON;i++){
        if(tabSalon[i] != NULL && strcmp(tabSalon[i]->nom,name) == 0){
            return i;
        }
    }
    return -1;
}

/**
 * @brief récupère le nom du salon ou se trouve un utilisateur
 * @param client l'id du client
 * @return le nom du salon
*/
char* recupNomSalonUser(int client){
    return tabSalon[tabdSC[client].id_salon]->nom; 
}

/**
 * @brief crée un salon
 * @param nom du salon
 * @param client le propriétaire du salon
 * @return true si salon créé, false sinon
*/
bool createSalon(char* nom,int client){
    int i = 0;
    bool found = false;
    while (i<NB_MAX_SALON && !found)
    {
        if(tabSalon[i] == NULL){
            found = true;
        }
        else{
            i++;
        }
    }
    if(found){
        tabSalon[i] = malloc(sizeof(struct salon));
        tabSalon[i]->nom = nom;
        tabSalon[i]->id = client;
        tabSalon[i]->NB_MAX = NB_MAX_PERSONNE_SALON;
        for(int j = 0;j<NB_MAX_PERSONNE_SALON;j++){
            tabSalon[i]->client[j] = -1;
        }
    }
    return found;
}

/**
 * @brief Ajoute un utilisateur a un salon
 * @param id du salon
 * @param client id du client
 * @return true si possible, false sinon
*/
bool AppendUserSalon(int id,int client){
    int i = 0;
    bool found = false;
    while(i<NB_MAX_PERSONNE_SALON && !found){
        if(tabSalon[id]->client[i] == -1){
            found = true;
            tabSalon[id]->client[i] = client;
            tabdSC[client].id_salon = id;
            //printf("id_salon : %d\n",tabdSC[client].id_salon);
        }
        i++;
    }
    return found;
}

/**
 * @brief enlève un utilisateur a un salon
 * @param id du salon
 * @param client id du salon
 * @return true si possible, false sinon
*/
void RemoveUserSalon(int id, int client){
    int i = 0;
    bool res = false;
    while(i<NB_MAX_PERSONNE_SALON && !res){
        if(tabSalon[id]->client[i] == client){
            res = true;
            tabSalon[id]->client[i] = -1;
        }
        i++;
    }
}

/**
 * @brief supprime le salon (et renvoie sur le salon main tout les utilisateurs sur le salon supprimé)
 * @param id du salon a supprimer
*/
void deleteSalon(int id){
    for(int i = 0;i<NB_MAX_PERSONNE_SALON;i++){
        if (tabSalon[id]->client[i] != -1){
            int id_client = tabSalon[id]->client[i];
            envoie(tabdSC[id_client].dSC,"Salon supprimé redirection vers le main");
            RemoveUserSalon(id,id_client);
            AppendUserSalon(0,id_client); //renvoie tout les clients sur le main
        }
    }
    free(tabSalon[id]);
    tabSalon[id] = NULL; 
}

/**
 * @brief compte le nombre de client dans un salon
 * @param id du salon
 * @return le nombre de client dans le salon
*/
int countNbClientSalon(int id){
    int count = 0;
    for(int i = 0;i<NB_MAX_PERSONNE_SALON;i++){
        if(tabSalon[id]->client[i] != -1){
            count++;
        }
    }
    return count;
}

/**
 * @brief compte le nombre de salon sur le serveur
 * @return le nombre de salon
*/
int countNbSalon(){
    int count = 0;
    for(int i = 0;i<NB_MAX_SALON;i++){
        if(tabSalon[i] != NULL){
            count++;
        }
    }
    return count;
}

/**
 * @brief récupère tout les utilisateurs d'un salon
 * @param id du salon
 * @return tableau contenant tout les id des users connectés
*/
int* getSalonUser(int id){
    return tabSalon[id]->client; 
}

/**
 * @brief recupère tout les pseudo des utilisateurs dans un salon
 * @param id du salon
 * @param nb nombre utilisateur
 * @return tableau contenant tout les noms des users connectés
*/
char** getSalonUserPseudo(int id,int nb){
    char** tab = malloc(sizeof(char*)*nb);
    int countTab = 0;
    int i = 0;
    while(countTab<nb){
        if(tabSalon[id]->client[i] != -1){
            tab[countTab] = tabdSC[tabSalon[id]->client[i]].pseudo; //récupère le pseudo 
            countTab++;
        }
        i++;
    }
    return tab; 
}

/**
 * @brief fonction qui récupère tout les noms des salons 
 * @param nb le nombre de salon
 * @return un tableau contenant tout les nom de salon
*/
char** getAllSalonName(int nb){
    char** tab = malloc(sizeof(char*)*nb);
    int countTab = 0;
    int i = 0;
    while(countTab<nb){
        if(tabSalon[i] != NULL){
            tab[countTab] = getSalonName(i); //récupère le pseudo 
            countTab++;
        }
        i++;
    }
    return tab; 
}

// /join NOM
// /create NOM
// /delete NOM
// /connected