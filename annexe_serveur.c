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

//Fonction qui permet de vérifier la char* entrée par l'utilisateur existe
bool verif_commande(char* msg,char* msg_commande){
    bool res = true;
    int i = 0;
    if ((strlen(msg)-1) == (strlen(msg_commande))){ //msg a -1 car on a le lanceur de commande devant
            while (i<(int)strlen(msg_commande) && msg[i] == '\0' && res ) {
            if (msg[i+1] == msg_commande[i]) { //i+1 car on ne regarde pas le lanceur de commande
                i++;
            }
            else {
                res = false;
            }
        }
    }
    return res;
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