#include "serveur.h"
#include <stdlib.h>
#include <stdbool.h>

#define NB_MAX_SALON 20 //limite max du nombre de salon 


struct salon* tabSalon[NB_MAX_SALON]; //tableau des salons

void initSalon(){
    for(int i; i<NB_MAX_SALON; i++){
        tabSalon[i] = NULL;
    }
}

//Crée un salon, si le salon est crée renvoie true, false sinon
bool createSalon(char* nom,int client){
    int i = 0;
    bool found = false;
    while (i<NB_MAX_SALON && found)
    {
        if(tabSalon[i] == NULL){
            found = false;
        }
        i++;
    }
    if(found){
        tabSalon[i] = malloc(sizeof(struct salon));
        tabSalon[i]->nom = nom;
        tabSalon[i]->id = client;
        tabSalon[i]->NB_MAX = NB_MAX_PERSONNE_SALON;
        for(int j = 0;j<NB_MAX_PERSONNE_SALON;i++){
            tabSalon[i]->client[j] == -1;
        }
    }
    return found;
}

//supprime un salon, si le salon précisé n'existe pas, ne se passe rien
void deleteSalon(int id){
    free(tabSalon[id]);
    tabSalon[id] = NULL; 
}

//Ajoute un utilisateur a un salon, si l'opération a réussi, alors renvoie true, sinon false
void AppendUserSalon(int id,int client){
    int i = 0;
    bool found = false;
    while(i<NB_MAX_PERSONNE_SALON && found){
        if(tabSalon[id]->client[i] == -1){
            found = true;
            tabSalon[id]->client[i] = client;
            tabdSC[client].id_salon = id;  
        }
        i++;
    }
    return found;
}

void RemoveUserSalon(int id, int client){
    int i = 0;
    bool found = false;
    while(i<NB_MAX_PERSONNE_SALON && found){
        if(tabSalon[id]->client[i] == -1){
            found = true;
            tabSalon[id]->client[i] = client;
        }
        i++;
    }
    return found;
}

int countNbClientSalon(int id){
    int count = 0;
    for(int i = 0;i<NB_MAX_PERSONNE_SALON;i++){
        if(tabSalon[id]->client[i] == -1){
            count++;
        }
    }
    return count;
}

int countNbSalon(){
    int count = 0;
    for(int i = 0;i<NB_MAX_PERSONNE_SALON;i++){
        if(tabSalon[i] != NULL){
            count++;
        }
    }
    return count;
}

//renvoie le tableau des utilisateurs du salon
int* getSalonUser(int id){
    return tabSalon[id]->client; 
}

//renvoie le tableau des pseudos des utilisateurs 
int* getSalonUserPseudo(int id,int* nb){
    *nb = countNbClientSalon(id);
    char* tab[*nb];
    int countTab = 0;
    int i =0;
    while(countTab<*nb){
        if(tabSalon[id]->client[i] != -1){
            tab[countTab] = tabdSC[tabSalon[id]->client[i]].pseudo; //récupère le pseudo 
        }
        i++;
    }
    return tab; 
}

//renvoie le nom du salon
char* getSalonName(int id){
    return tabSalon[id]->nom;
}

//renvoie l'id du client qui a créé le salon
int* getIdProp(int id){
    return tabSalon[id]->id;
}

int getNbMax(int id){
    return tabSalon[id]->NB_MAX;
}

//setteur qui mets a jour le nombre personne max dans le salon
bool setSalonNbMax(int id,int nb){
    if (nb<=NB_MAX_PERSONNE_SALON && nb > 1){
        tabSalon[id]->NB_MAX = nb;
        return true;
    }
    return false;
}

void setSalonName(int id,char* nom){
    tabSalon[id]->nom = nom;
}

void setSalonProp(int id, int new){
    tabSalon[id]->id = new;
}

int getIdSalon(char* name){
    for(int i = 0;i<NB_MAX_SALON;i++){
        if(strcmp(tabSalon[i]->nom,name) == 0){
            return i;
        }
    }
    return -1;
}
// /join NOM
// /create NOM
// /delete NOM
// /connected NOM
// /getSalon