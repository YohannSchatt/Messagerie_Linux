
#include <stdbool.h>
#include <stdlib.h>

//Fonction qui prend une adresse d'un string et met en premier caractère la fin de caractère
void setMsgVoid(char** msg){
    *msg[0] = '\0';
}

//Fonction qui reçoit un message du client associé au socket donnée en paramètre et le met dans message qui sera transmit a la fonction envoyer
//Entrée : le socket et le pointeur du message
//Sortie : un Booléen qui précise si on continu la communication, et met à jour le message
char* lecture(int dSC,bool* continu){
    bool res = true;
    int taille;
    int err = recv(dSC,&taille, sizeof(int), 0);
    if (err > 0){ //communication de la taille
        char* msg = (char*)malloc(taille*sizeof(char));
        err = recv(dSC,msg, taille, 0);
        if (err <= 0){ //reçoit le message
            continu = false;
            setMsgVoid(&msg);
        }
        else {
            return msg;
        }
    }
    else {
        continu = false;
    }
    return NULL;
}

//Fonction qui envoie le message donnée en paramètre avec le pseudo correspondant au client qu'on a en paramètre grâce au socket
//Entrée : le socket, le message, et le pseudo
//Sortie : renvoie rien, le message est envoyé
void envoie(int dSC,char* msg){
    int taille = strlen(msg)+1;
    if(send(dSC, &taille, sizeof(int), 0) != -1){
        send(dSC, msg, taille, 0);
    }
}