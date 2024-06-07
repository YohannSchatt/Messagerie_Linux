#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
/**
 * @brief Fonction qui permet de récupérer le pseudo dans une commande
 * @param msg le message
 * @param pos la position du début du pseudo
 * @return renvoie le pseudo
*/
char* recup_pseudo(char* msg,int pos){
    char* pseudo = (char*)malloc(16*sizeof(char)); //taille max de 16 pour un pseudo
    int i = 0;
    while( i < 16 && msg[pos+i] != ' ' && msg[pos+i] != '\0'){
        pseudo[i] = msg[pos+i];
        i++;
    }
    return pseudo;
}

/**
 * @brief Fonction qui permet de récupérer le pseudo dans une commande
 * @param msg la commande
 * @param pos la position de l'espace juste devant le message a récupérer
 * @return renvoie le message
*/
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

/**
 * @brief vérifie la commande si elle correspond bien
 * @param msg_commande commande dans le message
 * @param msg commande initial que le message doit respecter
 * @return renvoie un booléen, true si c'est bien la bonne commande, false sinon
*/
bool verif_commande(char* msg_commande,char* msg){
    bool res = false;
    int i = 1; //on ne regarde pas le /
    if ((strlen(msg)) >= (strlen(msg_commande))){
        res = true;
        while(i<(int)strlen(msg_commande) && i<(int)strlen(msg) && msg[i] != '\0' && msg[i] != ' ' && msg_commande[i] != '\0' && msg_commande[i] != ' ' && res){
            if (msg[i] == msg_commande[i]) {
                i++;
            }
            else {
                res = false;
            }
        }
        if (res && i <strlen(msg) && msg[i] != ' '){ //si le mot écrit commence pareil que le nom de la commande mais est plus grand
            res = false;
        }
    }
    return res;
}

/**
 * @brief récupère le nom du salon dans le message
 * @param msg le message 
 * @param pos la position ou se situe le début du nom du salon
 * @return renvoie le nom du salon
*/
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
/**
 * @brief Fonction qui fusionne le pseudo de la personne concerné, et le message principale avec des caractère qui les joints
 * @param msg le message
 * @param pseudo le pseudo
 * @param jointure les caractères de jointure
 * @return le message concaténé
*/
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

/**
 * @brief Fonction qui fusionne le pseudo de l'utilisateur donné en paramètre et le message donné en paramètre
 * @param msg le message
 * @param pseudo le pseudo de l'originaire du message
 * @return renvoie la chaine concaténé
*/
char* creation_msg_client_public(char* msg, char* pseudo) {
    return creation_msg_serveur(msg,pseudo," : ");
}

/**
 * @brief Fonction qui fusionne le pseudo de l'utilisateur donné en paramètre et le message donné en paramètre
 * @param msg le message
 * @param pseudo le pseudo de l'originaire du message
 * @return renvoie la chaine concaténé
*/
char* creation_msg_client_prive(char* msg, char* pseudo) {
    return creation_msg_serveur(msg,pseudo," (Message privé) : ");
}