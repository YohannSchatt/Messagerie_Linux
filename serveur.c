//BUT DU PROG : Ce programme est un serveur de chat qui accepte les connexions de deux clients simultanément et facilite la communication entre eux.
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include "file.c"
#include "annexe_serveur.c"
#include "CommandeSalon.c"

int  NB_PERSONNE_ACTUELLE = 0;//compteur du nombre de personne connecté
int PORT;


sem_t semaphore; //semaphore qui sert a la file d'attente

/**
 * @brief supprime le client du tableau et ferme son socket
 * @param dSC le socket a qui couper la connection
 * @param id l'id du client dans le tableau
*/
void fin_connexion(int dSC,int id) {

    pthread_mutex_lock(&M1); //empêche le tableau d'être accédé pour éviter problème d'exclusion mutuelle
    tabdSC[id].dSC = -1;
    pthread_mutex_unlock(&M1); //reouvre le tableau

    pthread_mutex_lock(&M2); //reouvre le nombre de personne
    NB_PERSONNE_ACTUELLE--;
    pthread_mutex_unlock(&M2); //empêche le nombre de personne présente d'être accédé pour éviter problème d'exclusion mutuelles
    shutdown(dSC,2); //ferme le socket

    sem_post(&semaphore);
    printf("fermeture\n");
}

/**
 * @brief Fonction qui envoie le message donné en paramètre a tout le monde  du salon sauf le client qui crée le message
 * @param id l'id du client qui envoie le message
 * @param msg le message a envoyer
*/
void envoie_everyone_client(int id,char* msg){
    pthread_mutex_lock(&M1); //on bloque l'accès au tableau
    int id_salon = tabdSC[id].id_salon;
    int* ClientInSalon = getSalonUser(id_salon);
    for(int i = 0;i<NB_MAX_PERSONNE_SALON;i++) {
        if (ClientInSalon[i] != -1 && id != ClientInSalon[i]) { //si le socket existe et est différent de celui de notre client alors on envoie le message
            envoie(tabdSC[ClientInSalon[i]].dSC, msg);
        }
    }
    pthread_mutex_unlock(&M1); //on redonne l'accès au tableau
}

/**
 * @brief Fonction utilisé uniquement par le serveur qui envoie un message donné en paramètre a tout le monde
 * @param msg le message a envoyer
*/
void envoie_everyone_serveur(char* msg){
    pthread_mutex_lock(&M1); //on bloque l'accès au tableau
    for(int i = 0;i<NB_MAX_PERSONNE;i++) {
        if (tabdSC[i].dSC != -1) { //si le socket existe et est différent de celui de notre client alors on envoie le message
            envoie(tabdSC[i].dSC, msg);
        }
    }
    pthread_mutex_unlock(&M1); //on redonne l'accès au tableau
}

/**
 * @brief Fonction qui arrête le programme et coupe tout les sockets
 * @param n inutile, sert juste pour le mettre dans un signal
*/
void ArretForce(int n) {
    printf("\33[2K\r");
    printf("Coupure du programme\n");
    envoie_everyone_serveur("fermeture du serveur");
    pthread_mutex_lock(&M1);
    for (int i = 0;i<NB_MAX_PERSONNE;i++) {
        if(tabdSC[i].dSC != -1){
            shutdown(tabdSC[i].dSC,2);
        }
    }
    pthread_mutex_unlock(&M1);
    exit(0);
}

/**
 * @brief Fonction qui permet l'envoie d'un message privé a un client 
 * @param msg le message a envoyer
 * @param pseudo le pseudo du client qui doit le recevoir
 * @param args le struct du client qui envoie le message
*/
void envoie_prive_client(char* msg,char* pseudo,struct mem_Thread args){
    int i = 0;
    bool envoye = false;
    pthread_mutex_lock(&M1); //on bloque l'accès au tableau
    printf("%s\n", pseudo);
    while (i<NB_MAX_PERSONNE && !envoye){
        if (tabdSC[i].dSC != -1) {
            printf("%s\n",tabdSC[i].pseudo);
        }
        if(tabdSC[i].dSC != -1 && strcmp(tabdSC[i].pseudo,pseudo) == 0){
            envoie(tabdSC[i].dSC,msg);
        }
        i++;
    }
    pthread_mutex_unlock(&M1); //on redonne l'accès au tableau
}

/**
 * @brief commande qui permet d'envoyer le manuel a l'utilisateur
 * @param dSC le socket du client a qui envoyer
*/
void envoyer_manuel(int dSC) {
    FILE *fichier;
    char ligne[256]; // Taille maximale d'une ligne du manuel
    fichier = fopen("manuel.txt", "r");
    if (fichier == NULL) {
        // Gérer l'erreur si le fichier n'a pas pu être ouvert
        printf("Erreur : Impossible d'ouvrir le fichier manuel.txt\n");
    } else {
        // Envoyer le contenu du fichier ligne par ligne
        pthread_mutex_lock(&M1);
        while (fgets(ligne, sizeof(ligne), fichier) != NULL) {
            envoie(dSC, ligne);
        }
        pthread_mutex_unlock(&M1);
        fclose(fichier);
    }
}

/**
 * @brief Fonction qui prend le message et décide ce qu'il doit faire avec
 * @param msg le message du client
 * @param args les infos du client qui a envoyé le message
 * @return un booléen, true si le thread doit se couper, false sinon
*/
bool protocol(char *msg, struct mem_Thread args){
    bool res = true;
    if (msg[0] == '@'){
        char* pseudo_client_recevoir = recup_pseudo(msg, 1);
        printf("%sz",pseudo_client_recevoir);
        char* contenu_msg = recup_message(msg,strlen(pseudo_client_recevoir)+1);
        char* message_complet = creation_msg_client_prive(contenu_msg,args.pseudo);
        envoie_prive_client(message_complet,pseudo_client_recevoir,args);
    }
    else if (msg[0] == '/') {
        if (strcmp("/help",msg) == 0) {
            envoyer_manuel(args.dSC);
        }
        else if (strcmp("/quitter",msg) == 0) {
            res = false;
        }
        else if (strcmp("/fermeture",msg) == 0){
            ArretForce(0);
        }
        else if (verif_commande("/join",msg)){
            char* nom = recupNomSalon(msg,6);
            join(args.id,nom);
        }
        else if (verif_commande("/create",msg)){
            char* nom = recupNomSalon(msg,8);
            create(args.id,nom);
        }
        else if (verif_commande("/delete",msg)){
            char* nom = recupNomSalon(msg,8);
            delete(args.id,nom);
        }
        else if (verif_commande("/getSalon",msg)){
            getSalon(args.id);
        }
        else if (verif_commande("/connected",msg)){
            char* nom = recupNomSalonUser(args.id);
            connected(args.id,nom);
        }
        else if (verif_commande("/kick",msg)){
            char* nom = recupNomSalon(msg,6); //utilisation de la même fonction de récupération du salon pour le speudo comme c'est le même principe
            int i = 0;
            bool found = false;
            pthread_mutex_lock(&M1);
            if(strcmp(nom,args.pseudo) == 0){
                envoie(tabdSC[args.id].dSC, "tu es maso ?");
                pthread_mutex_unlock(&M1);
            }
            else {
                while(i<NB_MAX_PERSONNE && !found){
                    if(strcmp(nom,tabdSC[i].pseudo) == 0){
                        found = true;
                        envoie(tabdSC[i].dSC,"Vous êtes kick !");
                        printf("test\n");
                        int rc = pthread_cancel(tabdSC[i].thread);
                        printf("zdzq\n");
                        if (rc) {
                            printf("Erreur : impossible d'annuler le thread %d, code d'erreur: %d\n",i,rc);
                        }
                        pthread_mutex_unlock(&M1);
                        printf("a\n");
                        fin_connexion(tabdSC[i].dSC,i);
                    }
                    i++;
                }
                if(!found){
                    envoie(tabdSC[args.id].dSC,"l'utilisateur n'existe pas");
                    pthread_mutex_unlock(&M1);
                }
            }
        }
        else {
            envoie(args.dSC, "Commande inconnu faite /help pour plus d'information");
        }
    }
    else {
        char* message_complet = creation_msg_client_public(msg,args.pseudo);
        envoie_everyone_client(args.id,message_complet);
    }
    printf("sfsefef\n");
    return res;
}

/**
 * @brief Fonction qui gère la lecture du message d'un client qui sera ensuite envoyé a tout les clients
 * @param args struct d'argument pour le thread, contenant le socket du client, le pseudo et l'id du client dans le tableau
*/
void lecture_envoie(struct mem_Thread args) {
    bool continu = true; //booléen qui va assurer la boucle tant que la communication n'est pas coupé 
    while (continu) {
        char* msgrecu = lecture(args.dSC, &continu); //cas d'erreur de l'envoi, change la valeur de continu
        if (continu){
            continu = protocol(msgrecu,args);
        }
    }
    //message de fin de communication

    char* msgcomplet = creation_msg_serveur("a quitté le serveur",args.pseudo," ");
    envoie_everyone_client(args.dSC,msgcomplet);
    fin_connexion(args.dSC,args.id); //si communication coupé alors on mets fin au socket
}

/**
 * @brief Fonction qui a pour but d'initialiser le socket de connexion initial
 * @param port le port donnée en argument du serveur quand on lance le programme
 * @return le socket initial de communication
*/
int init_ouverture_connexion(int port) {
    
    int dS = socket(PF_INET, SOCK_STREAM, 0); //crée le socket en TCP
    if (dS == -1){
        fprintf(stderr,"erreur lors de la création du socket");
        exit(0);
    }
    else {
       printf("Socket Créé\n");
        struct sockaddr_in ad; // structure des sockets
        ad.sin_family = AF_INET;
        ad.sin_addr.s_addr = INADDR_ANY ;
        ad.sin_port = htons(port) ;
        if (bind(dS, (struct sockaddr*)&ad, sizeof(ad)) == -1) { //Donne un nom au socket
            shutdown(dS,2);
            fprintf(stderr,"problème de nommage du socket\n");
            exit(0);
        }
        else {
            printf("Socket Nommé\n");
            if(listen(dS, 7) == -1){  //mets en position d'écoute
                shutdown(dS,2);
                fprintf(stderr,"problème à initialiser l'écoute\n");
                exit(0);
            }
            else {
                printf("Mode écoute\n");
                return dS;
            }
        }
    }
}

/**
 * @brief Fonction qui vérifie que le pseudo donné en paramètre n'est pas déjà pris
 * @param pseudo a verifier
 * @return booléen, true si le pseudo n'est pas pris, false sinon
*/
bool verif_pseudo(char* pseudo){
    bool res = true;
    int i = 0;
    pthread_mutex_lock(&M1);
    while (i<NB_MAX_PERSONNE && res){
        if(tabdSC[i].dSC != -1 && strcmp(tabdSC[i].pseudo,pseudo) == 0){
            res = false;
        }
        i++;
    }
    pthread_mutex_unlock(&M1);
    return res;
}

/**
 * @brief Fonction qui permet a l'utilisateur de prendre un pseudo
 * @param args_thread donné qui permet de récuperer le socket du client (void* car argument phtread_create)
*/
void* choixPseudo(void* args_thread){
    struct Args_Thread th = *((struct Args_Thread*)args_thread); //le socket client

    struct mem_Thread args; //crée la stucture permettant de mettre en des arguments a la fonction envoie_lecture

    int taille;

    bool continu = true;

    int err;

    while(continu){
        if(recv(th.dSC,&taille, sizeof(int), 0) <= 0){ //reçoit la taille du message
            shutdown(th.dSC,2);
            perror("erreur recv taille\n");
            pthread_exit(0);
        } 
        args.pseudo = (char*)malloc(taille*sizeof(char));
        if (args.pseudo == NULL){
            shutdown(th.dSC,2);
            perror("erreur allocation mémoire pseudo\n");
            pthread_exit(0);
        }
        if(recv(th.dSC, args.pseudo, sizeof(char)*taille, 0) <= 0){ //reçoit le message
            shutdown(th.dSC,2);
            perror("erreur réception pseudo\n");
            pthread_exit(0);
        } 
        char* pos = (char*)malloc(sizeof(char));
        if (pos = NULL){
            shutdown(th.dSC,2);
            perror("erreur allocation pos\n");
            pthread_exit(0);
        }
        pos = strchr(args.pseudo,' ');
        if (strlen(args.pseudo)> 0 && args.pseudo[0] != ' '){ //vérifie si le premier caractère n'est pas un espace
            if (pos != NULL){ //propriété de la fonction strchr renvoie NULL si il n'y a pas le caratère demandé
                *pos = '\0';
            }
            if (verif_pseudo(args.pseudo)){
                continu = false;
            }
        }
        if(send(th.dSC,&continu, sizeof(bool), 0) <= 0){
            shutdown(th.dSC,2);
            perror("erreur envoie continu\n");
            pthread_exit(0);
        }
    }



    args.id = th.id; //la position du socket du client dans le tableau
    args.dSC = th.dSC;

    pthread_mutex_lock(&M1);    

    tabdSC[args.id].dSC = args.dSC;
    tabdSC[args.id].pseudo = args.pseudo;

    pthread_mutex_unlock(&M1); 

    char* msgcomplet = creation_msg_serveur("a rejoint le serveur",args.pseudo," ");
    envoie_everyone_serveur(msgcomplet);

    lecture_envoie(args); //le client va pouvoir commencer a communiquer
}

/**
 * @brief initie le socket pour l'envoie des fichiers
 * @param port le port qui permet l'envoie des fichiers ou le socket sera initialisé
 * @return le socket initial des communications des fichiers
*/
int initSocketFile(int port){
    int dSF = socket(PF_INET, SOCK_STREAM, 0); //crée le socket en TCP
    if (dSF == -1){
        fprintf(stderr,"erreur lors de la création du socket");
        pthread_exit(0);
    }
    else {
       printf("Socket Créé\n");
        struct sockaddr_in ad; // structure des sockets
        ad.sin_family = AF_INET;
        ad.sin_addr.s_addr = INADDR_ANY ;
        ad.sin_port = htons(port) ;
        if (bind(dSF, (struct sockaddr*)&ad, sizeof(ad)) == -1) { //Donne un nom au socket
            shutdown(dSF,2);
            fprintf(stderr,"problème de nommage du socket\n");
            pthread_exit(0);
        }
        else {
            printf("Socket Nommé\n");
            if(listen(dSF, 7) == -1){  //mets en position d'écoute
                shutdown(dSF,2);
                fprintf(stderr,"problème à initialiser l'écoute\n");
                pthread_exit(0);
            }
            else {
                printf("Mode écoute\n");
                return dSF;
            }
        }
    }
}

/**
 * @brief qui permet de choisir et d'afficher les fichier disponibles dans le serveur a récupérer
 * @param dSFC le socket du serveur pour les fichiers
*/
char* Interface_choix_fichier_getFile(int dSFC) {
    int file_count;
    char** filenames = getFileInFolder("./file_serveur", &file_count);
    char* msg = (char*)malloc(sizeof(char)*16);
    int err = send(dSFC,&file_count,sizeof(int),0);
    if (err <= 0){
        printf("Erreur connexion fichier !");
        finFichier(dSFC);
    }
    int taille;
    for(int i = 0; i<file_count;i++){
        taille = strlen(filenames[i])+1;
        err = send(dSFC,&taille,sizeof(int),0);
        if (err <= 0){
            printf("Erreur envoie taille nom fichier !\n");
            finFichier(dSFC);
        }
        err = send(dSFC,filenames[i],sizeof(char)*taille,0);
        if (err <= 0){
            printf("Erreur envoie nom fichier !\n");
            finFichier(dSFC);
        }
    }
    int choix;
    err = recv(dSFC,&choix,sizeof(int),0);
    if (err <= 0){
        printf("Erreur reception choix !\n");
        finFichier(dSFC);
    }
    if (choix < file_count && choix >= 0){
        char* nameFile = strdup(filenames[choix]);
        for (int j = 0; j < file_count; j++) {
            free(filenames[j]);
        }
        return nameFile;
    }
    else {
        printf("Choix invalide.\n");
        for (int j = 0; j < file_count; j++) {
            free(filenames[j]);
        }
        return NULL;
    }
}

/**
 * @brief Lance la commande getFile, pour récuperer un fichier au serveur
 * @param args le dSFC du client
*/
void* getFile(void* args){
    int dSFC = *((int*)args);
    recvFichier(dSFC,"./file_serveur/");
}

/**
 * @brief Lance la commande sendFile, pour envoyer un fichier au serveur
 * @param args le dSFC du client
*/
void* sendFile(void* args){
    int dSFC = *((int*)args);
    char* nameFile = Interface_choix_fichier_getFile(dSFC);
    if (nameFile == NULL){
        finFichier(dSFC);
    }
    sendFichier(nameFile,"./file_serveur/",dSFC);
}

/**
 * @brief Lance le thread de communication pour que le client se connecte et lance soit sendFile, soit getFile
 * @param args le socket initial du serveur file
*/
void* thread_file(void * args){

    int dSF = *((int*)args);

    struct sockaddr_in aC ; //structure du socket
    socklen_t lg = sizeof(struct sockaddr_in);

    while(2==2){
        int dSFC = accept(dSF, (struct sockaddr*) &aC,&lg); //crée le socket client
        if (dSFC == -1) { //gestion de l'erreur de accept
            printf("problème de connexion\n");
        }
        int a = 1;
        int err = send(dSFC,&a, sizeof(int), 0); //accusé de connexion
        if (err <= 0){
            printf("Erreur connexion fichier !");
            finFichier(dSFC);
        }
        err = recv(dSFC,&a, sizeof(int),0); //0 pour sendFile, 1 pour getFile
        if (err <= 0){
            printf("Erreur connexion fichier !");
            finFichier(dSFC);
        }
        pthread_t thread_file_client;
        if (a == 0){ //pour sendFile
            pthread_create(&thread_file_client, NULL, getFile,(void*)(&dSFC));
        }
        else { //pour getFile
            pthread_create(&thread_file_client, NULL, sendFile,(void*)(&dSFC));
        };
    }
}


/**
 * @brief lance la connexion pour que le client puisse arriver sur le serveur
 * @param args le socket du serveur
*/
void* init_connexion(void* args) {

    int dS = (*(int*)args);

    for(int i = 0; i<NB_MAX_PERSONNE;i++) { //initialise le tableau
        tabdSC[i].dSC = -1;
    }

    struct sockaddr_in aC ; //structure du socket
    socklen_t lg = sizeof(struct sockaddr_in) ; 

    sem_init(&semaphore, 0, NB_MAX_PERSONNE); 
    initSalon();
    createSalon("main",-1); //création du salon principale qui appartient a personne
    int a = 1;
    while(true){ //continue a s'éxecuter 
        sem_wait(&semaphore);
        int dSC = accept(dS, (struct sockaddr*) &aC,&lg); //crée le socket client
        if (dSC == -1) { //gestion de l'erreur de accept
            printf("problème de connexion\n");
        }
        else {
            send(dSC,&a, sizeof(int), 0);
            pthread_mutex_lock(&M2); //bloque l'accès au compteur du nombre de personne
            printf("Client Connecté\n");            
            NB_PERSONNE_ACTUELLE++;

            pthread_mutex_unlock(&M2); //redonne l'accès au nombre de client connecté

            pthread_t thread;

            pthread_mutex_lock(&M1); //bloque l'accès au tableau
            int i = 0;
            while(tabdSC[i].dSC != -1){ //si on trouve un slot de libre (qui existe forcément par la vérification fait au préalable)
                i = (i + 1)%NB_MAX_PERSONNE;
            }

            tabdSC[i].id = i;
            pthread_mutex_unlock(&M1); //on redonne l'accès

            struct Args_Thread args;
            args.dSC = dSC;
            args.id = i;

            if (!AppendUserSalon(0,args.id)){ //le salon main sera toujours sur 0
                printf("erreur ajout a un salon\n");
            }

            pthread_create(&tabdSC[i].thread, NULL, choixPseudo,(void*)&args); //on lance le thread du client
        }
    }
}

/**
 * @brief programme principale qui lance le serveur communication et le serveur fichier
*/
int main(int argc, char *argv[]) { 

    printf("Début programme\n");

    signal(SIGINT, ArretForce); //Ajout du signal de fin ctrl+c

    PORT = atoi(argv[1]);

    int dS = init_ouverture_connexion(PORT); //on crée le socket de communication 
    int dSF = initSocketFile(PORT+1);
    
    pthread_t th_communication;
    pthread_t th_file;

    pthread_create(&th_communication,NULL,init_connexion,(void*)(&dS));
    pthread_create(&th_file,NULL,thread_file,(void*)(&dSF));

    pthread_join(th_communication,NULL);

    shutdown(dS,2); //fin du socket de communication
    printf("fin programme");
}
