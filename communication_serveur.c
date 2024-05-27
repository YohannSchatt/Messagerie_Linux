//Fonction qui reçoit un message du client associé au socket donnée en paramètre et le met dans message qui sera transmit a la fonction envoyer
//Entrée : le socket et le pointeur du message
//Sortie : un Booléen qui précise si on continu la communication, et met à jour le message
bool lecture(int dSC,char **msg){
    bool res = true;
    int taille;
    int err = recv(dSC,&taille, sizeof(int), 0);
    if (err != -1 && err != 0){ //communication de la taille
        *msg = (char*)malloc(taille*sizeof(char));
        err = recv(dSC,*msg, taille, 0);
        if (err == -1 && err == 0){ //reçoit le message
            res = false;
        }
    }
    else {
        res = false;
    }
    return res;
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