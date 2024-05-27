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
#include "communication_client.c"

#define NB_MAX_PERSONNE 3 //limite max de personne sur le serveur
#define MAX_FILE 200 //limite max de personne sur le serveur
#define NB_MAX_SALON 20

int  NB_PERSONNE_ACTUELLE = 0;//compteur du nombre de personne connecté
int PORT;
 
pthread_mutex_t M1 = PTHREAD_MUTEX_INITIALIZER; //mutex qui protège l'accès au tableau des sockets clients
pthread_mutex_t M2 = PTHREAD_MUTEX_INITIALIZER; //mutex qui protège l'accès au nombre  des sockets clients

sem_t semaphore; //semaphore qui sert a la file d'attente

struct mem_Thread { //structure permettant de transférer les arguments dans les différents threads
    int id; //l'id pour retrouver les éléments dans les tableaux
    int dSC; //le socket du client
    char* pseudo; //son pseudo
};

struct Args_Thread {
    int id; //l'id pour retrouver les éléments dans les tableaux
    int dSC; //le socket du client
};

struct client {
    int dSC; //socket du client
    char* pseudo; //son pseudo
    pthread_t thread; //son thread
};

struct client tabdSC[NB_MAX_PERSONNE+2]; //tableau des sockets des clients

struct salon tabSalon[NB_MAX_SALON];

//Fonction qui prend en paramètre un socket et l'id dans le tableau
// elle va supprimer le client du tableau puis fermer le socket
//Entrée : le socket et l'id
//Sortie : tableau des sockets modifié
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

//fonction qui arrête le programme et coupe tout les sockets
void ArretForce(int n) {
    printf("\33[2K\r");
    printf("Coupure du programme\n");
    envoie_everyone_serveur("fermeture du serveur");
    pthread_mutex_lock(&M1);
    for (int i = 0;i<NB_MAX_PERSONNE+1;i++) {
        shutdown(tabdSC[i].dSC,2);
    }
    pthread_mutex_unlock(&M1);
    exit(0);
}

//Fonction qui envoie message donné en paramètre a tout le monde sauf le client qui a crée le message 
//Entrée : le socket du client qui envoie le message
//Sortie : renvoie rien, a envoyé le message à tout le monde sauf le client a l'origine du message
void envoie_everyone_client(int dSC,char* msg){
    pthread_mutex_lock(&M1); //on bloque l'accès au tableau
    for(int i = 0;i<NB_MAX_PERSONNE;i++) {
        if (tabdSC[i].dSC != -1 && dSC != tabdSC[i].dSC) { //si le socket existe et est différent de celui de notre client alors on envoie le message
            envoie(tabdSC[i].dSC, msg);
        }
    }
    pthread_mutex_unlock(&M1); //on redonne l'accès au tableau
}

//Fonction utilisé uniquement par le serveur qui envoie un message donné en paramètre a tout le monde 
//Entrée : un String (le message)
//Sortie : renvoie rien, envoie le message à tout le monde
void envoie_everyone_serveur(char* msg){
    pthread_mutex_lock(&M1); //on bloque l'accès au tableau
    for(int i = 0;i<NB_MAX_PERSONNE;i++) {
        if (tabdSC[i].dSC != -1) { //si le socket existe et est différent de celui de notre client alors on envoie le message
            envoie(tabdSC[i].dSC, msg);
        }
    }
    pthread_mutex_unlock(&M1); //on redonne l'accès au tableau
}

//fonction qui permet d'envoyer un message privé a un autre client
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

// Ajouter cette fonction pour envoyer le contenu de manuel.txt
void envoyer_manuel(int dSC) {
    FILE *fichier;
    char ligne[256]; // Taille maximale d'une ligne du manuel
    fichier = fopen("manuel.txt", "r");
    if (fichier == NULL) {
        // Gérer l'erreur si le fichier n'a pas pu être ouvert
        printf("Erreur : Impossible d'ouvrir le fichier manuel.txt\n");
    } else {
        // Envoyer le contenu du fichier ligne par ligne
        while (fgets(ligne, sizeof(ligne), fichier) != NULL) {
            envoie(dSC, ligne);
        }
        fclose(fichier);
    }
}

//cette fonction permet en fonction du message reçu d'éxecuter une action particulière
bool protocol(char *msg, struct mem_Thread args){
    bool res = true;
    if (msg[0] == '@'){
        char* pseudo_client_recevoir = recup_pseudo(msg, 1);
        char* contenu_msg = recup_message(msg,strlen(pseudo_client_recevoir)+2);
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
        else {
            envoie(args.dSC, "Commande inconnu faite /help pour plus d'information");
        }
    }
    else {
        char* message_complet = creation_msg_client_public(msg,args.pseudo);
        envoie_everyone_client(args.dSC,message_complet);
    }
    return res;
}

//Cette fonction gère la lecture du message d'un client qui sera ensuite envoyé a tout les clients
//Entrée : une struct d'argument pour le thread, contenant le socket du client, et le pseudo et l'id dans le tableau des sockets client
//Sortie : renvoie rien, mais assure la liaison entre les clients tant que le client du socket ne coupe pas la communication
void lecture_envoie(struct mem_Thread args) {
    bool continu = true; //booléen qui va assurer la boucle tant que la communication n'est pas coupé 
    char* msgrecu = (char*)malloc(sizeof(char));
    msgrecu[0] ='\0'; 
    while (continu) {
        continu = lecture(args.dSC, &msgrecu); //cas d'erreur de l'envoi, change la valeur de continu
        if (continu){
            continu = protocol(msgrecu,args);
        }
    }
    free(msgrecu);

    //message de fin de communication

    char* msgcomplet = creation_msg_serveur("a quitté le serveur",args.pseudo," ");
    envoie_everyone_client(args.dSC,msgcomplet);
    fin_connexion(args.dSC,args.id); //si communication coupé alors on mets fin au socket
}

//Fonction qui a pour but d'initialiser le socket de connexion initial
//Entrée : Le port donnée en argument du serveur quand on lance le programme
//Sortie : le socket inital de communication
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

//fonction qui vérifie si le pseudo n'est pas utilisé
bool verif_pseudo(char* pseudo){
    bool res = true;
    int i = 0;
    while (i<NB_MAX_PERSONNE && res){
        if(tabdSC[i].dSC != -1 && strcmp(tabdSC[i].pseudo,pseudo) == 0){
            res = false;
        }
        i++;
    }
    return res;
}

//fonction qui permet a l'utilisateur de prendre un pseudo valide
void* choixPseudo(void* args_thread){
    struct Args_Thread th = *((struct Args_Thread*)args_thread); //le socket client

    struct mem_Thread args; //crée la stucture permettant de mettre en des arguments a la fonction envoie_lecture

    int taille;

    bool continu = true;

    while(continu){
        recv(th.dSC,&taille, sizeof(int), 0); //reçoit la taille du message
        args.pseudo = (char*)malloc(taille*sizeof(char)); 
        recv(th.dSC, args.pseudo, taille, 0); //reçoit le message
        char* pos = (char*)malloc(sizeof(char));
        pos = strchr(args.pseudo,' ');
        if (strlen(args.pseudo)> 0 && args.pseudo[0] != ' '){ //vérifie si le premier caractère n'est pas un espace
            if (pos != NULL){ //propriété de la fonction strchr renvoie NULL si il n'y a pas le caratère demandé
                *pos = '\0';
            }
            if (verif_pseudo(args.pseudo)){
                continu = false;
            }
        }
        send(th.dSC,&continu, sizeof(bool), 0);
    }



    args.id = th.id; //la position du socket du client dans le tableau
    args.dSC = th.dSC;

    tabdSC[args.id].dSC = args.dSC;
    tabdSC[args.id].pseudo = args.pseudo; 

    char* msgcomplet = creation_msg_serveur("a rejoint le serveur",args.pseudo," ");
    envoie_everyone_serveur(msgcomplet);

    lecture_envoie(args); //le client va pouvoir commencer a communiquer
    pthread_exit(0);
}

//Crée un socket qui sera sur un port donnée en paramètre afin de recevoir des clients
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

//permet d'envoyer tout les fichiers au client afin qu'ils choisissent et reçoit la réponse du client
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

//lance la reception de fichier du serveur
void* getFile(void* args){
    int dSFC = *((int*)args);
    recvFichier(dSFC,"./file_serveur/");
}

//lance l'envoie du fichier au client
void* sendFile(void* args){
    int dSFC = *((int*)args);
    char* nameFile = Interface_choix_fichier_getFile(dSFC);
    if (nameFile == NULL){
        finFichier(dSFC);
    }
    sendFichier(nameFile,"./file_serveur/",dSFC);
}

//fonction qui a pour but d'éxecuter le thread de sendFile ou de getFile
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


//initialise la communication entre le client et le serveur
//Entrée : le socket d'écoute
//Sortie : renvoie rien, lance la fonction du pseudo avec le socket créé si la communication peut s'opérer
void* init_connexion(void* args) {

    int dS = (*(int*)args);

    for(int i = 0; i<NB_MAX_PERSONNE;i++) { //initialise le tableau
        tabdSC[i].dSC = -1;
    }

    struct sockaddr_in aC ; //structure du socket
    socklen_t lg = sizeof(struct sockaddr_in) ; 

    sem_init(&semaphore, 0, NB_MAX_PERSONNE); 
    int a = 1;
    while(true){ //continue a s'éxecuter 
        sem_wait(&semaphore);
        int dSC = accept(dS, (struct sockaddr*) &aC,&lg); //crée le socket client
        printf("%d\n",dSC);
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

            tabdSC[i].thread;
            pthread_mutex_unlock(&M1); //on redonne l'accès

            struct Args_Thread args;
            args.dSC = dSC;
            args.id = i;

            pthread_create(&thread, NULL, choixPseudo,(void*)&args); //on lance le thread du client
        }
    }
}

//main de la fonciton
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