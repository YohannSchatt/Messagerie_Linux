#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
//Fonction qui permet de récupérer le pseudo dans une commande
//Entrée : l'adresse du msg de la commande, la position de l'espace juste devant le pseudo
//Sortie : le pseudo de l'utilisateur 
char* recup_pseudo(char* msg,int pos){
    char* pseudo = (char*)malloc(16*sizeof(char)); //taille max de 16 pour un pseudo
    int i = 0;
    while( i < 16 && msg[pos+i] != ' ' && msg[pos+i] != '\0'){
        pseudo[i] = msg[pos+i];
        i++;
    }
    return pseudo;
}

//Fonction qui permet de récupérer le pseudo dans une commande 
//Entrée : l'adresse du msg de la commande, la position de l'espace juste devant le message
//Sortie : le message écrit par l'utilisateur 
char* recup_message(char* msg, int pos){
    int count = 0;
    while(msg[pos+count] != '\0'){
        count++;
    }
    char* message = (char*)malloc(count*sizeof(char));
    for(int i = 0; i < count ; i++){
        message[i] = msg[pos+i];
    }
    return message;
}

bool verif_commande(char* msg_commande,char* msg){
    bool res = true;
    int i = 1; //on ne regarde pas le /
    if ((strlen(msg)-1) >= (strlen(msg_commande)) && strlen(msg) > 1){
        while(i<(int)strlen(msg_commande) && i<(int)strlen(msg) && res){
            if (msg[i] == msg_commande[i]) {
                i++;
            }
            else {
                res = false;
            }
        }
    }
    return res;
}

char* recupNomSalon(char* msg,int pos){
    int count = 0;
    while(msg[pos+count] != ' ' && msg[pos+count] != '\0'){
        count++;
    }
    char* nom = (char*)malloc(count*sizeof(char));
    for(int i = 0; i < count ; i++){
        nom[i] = msg[pos+i];
    }
    return nom;
}

//Cette fonction fusionne le pseudo de la personne concerné, et le message principale avec des caractères qui les joints
//Entrée : trois String (un pseudo, un message, et un la jointure)
//Sortie : renvoie l'adresse d'un String avec comme forme "pseudo jointure message"
char* creation_msg_serveur(char* msg, char* pseudo,char* jointure) {
    int taillemsg = strlen(msg); 
    int taillepseudo = strlen(pseudo);
    int taillejointure = strlen(jointure);
    char* message = (char*)malloc((taillemsg+taillepseudo+taillejointure+1)*sizeof(char)); //taille du msg (+1 pour '\0)
    strcat(message,pseudo);
    strcat(message,jointure);
    strcat(message,msg);
    return message;
}

//Cette fonction fusionne le pseudo de l'utilisateur donné en paramètre et le message donné aussi en paramètre
//Entrée : deux String (un pseudo et un message)
//Sortie : renvoie l'adresse d'un String avec comme forme "pseudo : message"
char* creation_msg_client_public(char* msg, char* pseudo) {
    return creation_msg_serveur(msg,pseudo," : ");
}

char* creation_msg_client_prive(char* msg, char* pseudo) {
    return creation_msg_serveur(msg,pseudo," (Message privé) : ");
}