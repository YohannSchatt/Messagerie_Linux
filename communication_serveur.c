
#include <stdbool.h>
#include <stdlib.h>

/**
 * @brief Fonction qui prend une adresse d'un string et met en premier caractère la fin de cractère
 * @param msg le message a modifier
 * @return la chaine est directement modifié
*/
void setMsgVoid(char** msg){
    *msg = NULL;
}

/**
 * @brief Fonction qui reçoit un message du client associé
 * @param dSC le socket 
 * @param continu le booléen qui assure la continuité du thread
 * @return un booléen qui précise si il continu la communication
*/
char* lecture(int dSC,bool* continu){
    bool res = true;
    int taille;
    int err = recv(dSC,&taille, sizeof(int), 0);
    if (err > 0){ //communication de la taille
        char* msg = (char*)malloc(taille*sizeof(char));
        err = recv(dSC,msg, taille, 0);
        if (err <= 0){ //reçoit le message
            puts("caca");
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

/**
 * @brief Fonction qui envoie le message donnée a un client
 * @param dSC le socket du client a qui envoyer
 * @param msg le message a envoyer
*/
void envoie(int dSC,char* msg){
    int taille = strlen(msg)+1;
    if(send(dSC, &taille, sizeof(int), 0) < 0){
        perror("envoie msg problème");
        pthread_exit(0);
    }
    if(send(dSC, msg, taille, 0) < 0){
        perror("envoie msg problème");
        pthread_exit(0);
    }
}