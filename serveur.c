#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>

#define NB_MAX_PERSONNE 100 //limite max de personne sur le serveur
int  NB_PERSONNE_ACTUELLE = 0;//compteur du nombre de personne connecté

pthread_mutex_t M1 = PTHREAD_MUTEX_INITIALIZER; //mutex qui protège l'accès au tableau des sockets clients
pthread_mutex_t M2 = PTHREAD_MUTEX_INITIALIZER; //mutex qui protège l'accès au nombre  des sockets clients

struct Args_Thread { //structure permettant de transférer les arguments dans les différents threads
    int id; //l'id pour retrouver les éléments dans les tableaux
    int dSC; //le socket du client
    char* pseudo; //son pseudo
};

struct Args_Thread tabdSC[NB_MAX_PERSONNE+1]; //tableau des sockets des clients

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
    printf("fermeture\n");
}

//Fonction qui reçoit un message du client associé au socket donnée en paramètre et le met dans message qui sera transmit a la fonction envoyer
//Entrée : le socket et le pointeur du message
//Sortie : un Booléen qui précise si on continu la communication, et met à jour le message
bool lecture(int dSC,char **msg){
    bool res = true;
    int taille;
    if (recv(dSC,&taille, sizeof(int), 0) != -1){ //communication de la taille
        *msg = (char*)malloc(taille*sizeof(char));
        if (recv(dSC,*msg, taille, 0) != -1){ //reçoit le message
            if((strcmp(*msg,"fin") == 0)){ //si on reçoit fin alors on change le booléen pour couper la communication
                res = false;
            }
        }
        else {
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
    send(dSC, &taille, sizeof(int), 0);
    send(dSC, message, taille, 0); 
}

void envoie_everyone(int dSC,char* msg){
    for(int i = 0;i<NB_MAX_PERSONNE;i++) {
        pthread_mutex_lock(&M1); //on bloque l'accès au tableau
        if (tabdSC[i].dSC != -1 && args.dSC != tabdSC[i].dSC) { //si le socket existe et est différent de celui de notre client alors on envoie le message
            envoie(tabdSC[i].dSC, &msg,args.pseudo);
        }
        pthread_mutex_unlock(&M1); //on redonne l'accès au tableau
    }
}

void envoie_prive(int dSC, char* msg,char* pseudo){
    for(int i = 0;i<NB_MAX_PERSONNE;i++){
        pthread_mutex_lock(&M1); //on bloque l'accès au tableau
        if (tabdSC[i].dSC != -1 && args.dSC != tabdSC[i].dSC) { //si le socket existe et est différent de celui de notre client alors on envoie le message
            envoie(tabdSC[i].dSC, &msg,args.pseudo);
        }
        pthread_mutex_unlock(&M1); //on redonne l'accès au tableau
    }
}

//Fonction qui permet de récupérer le pseudo dans une commande
//Entrée : l'adresse du msg de la commande, la position de l'espace juste devant le pseudo
//Sortie : le pseudo de l'utilisateur 
char* recup_pseudo(char** msg,int pos){
    char* pseudo = (char*)malloc(16*sizeof(char)); //taille max de 16 pour un pseudo
    int i = 0;
    while( i < 16 && msg[pos+i] != ' ' && msg[pos+i] != '\0'){
        pseudo = msg[pos+i];
    }
    return pseudo;
}

char* recup_message(char** msg, int pos){
    int count = 0;
    while(msg[pos+count] != '\0'){
        count++;
    }
    char* message = (char*)malloc(count*sizeof(char));
    for(int i = 0; i < count ; i++){
        message = msg[pos+i];
    }
    return message;
}

int protocol(char *msg){
    char pos = msg[0];
    if (pos = '@'){
        if verif_commande(msg,"everyone") {
            envoie_all();
        }
        else if verif_commande(msg,"mp"){
            recup_pseudo();
            envoie_prive();
        }
    }
    else if (pos = '/') {
        if verif_commande(msg,"end") {
            fin_thread();
        }
        else {
            envoie_prive()
        }
    }
}

//Fonction qui permet de vérifier la char* entrée par l'utilisateur existe
bool verif_commande(char* msg,char* msg_commande){
    bool res = true;
    int i = 0;
    if ((strlen(msg)-1) == (strlen(msg_commande))){ //msg a -1 car on a le lanceur de commande devant
            while (i<strlen(msg_commande) && msg[i] == '\0' && res ) {
            if msg[i+1] == msg_commande[i] { //i+1 car on ne regarde pas le lanceur de commande
                i++;
            }
            else {
                res = false;
            }
        }
    }
    return res;
}

void creation_msg(char** msg, char* pseudo) {
    int taillemsg = strlen(*msg)+1; //taille du msg (+1 pour '\0)
    int taillepseudo = strlen(pseudo);
    char* message = (char*)malloc((taillemsg+taillepseudo+3)*sizeof(char));
    strcat(message,pseudo);
    strcat(message," : ");
    strcat(message, *msg);
}

//Cette fonction gère la lecture du message d'un client qui sera ensuite envoyé a tout les clients
//Entrée : une struct d'argument pour le thread, contenant le socket du client, et le pseudo et l'id dans le tableau des sockets client
//Sortie : renvoie rien, mais assure la liaison entre les clients tant que le client du socket ne coupe pas la communication
void lecture_envoie(struct Args_Thread args) {
    bool continu = true; //booléen qui va assurer la boucle tant que la communication n'est pas coupé 
    char* msg = (char*)malloc(sizeof(char));
    msg[0] ='\0'; 
    while (continu) {
        continu = lecture(args.dSC, &msg);
        if (continu){
            for(int i = 0;i<NB_MAX_PERSONNE;i++) {
                pthread_mutex_lock(&M1); //on bloque l'accès au tableau
                if (tabdSC[i] != -1 && args.dSC != tabdSC[i]) { //si le socket exite et est différent de celui de notre client alors on envoie le message
                    envoie(tabdSC[i], &msg,args.pseudo);
                }
                pthread_mutex_unlock(&M1); //on redonne l'accès au tableau
            }
        }
    }
    free(msg);
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

void* choixPseudo(void* args_thread){
    int dSC = *((int*)args_thread); //le socket client

    struct Args_Thread args; //crée la stucture permettant de mettre en des arguments a la fonction envoie_lecture

    int taille;
    recv(dSC,&taille, sizeof(int), 0); //reçoit la taille du message
    args.pseudo = (char*)malloc(taille*sizeof(char)); 
    recv(dSC, args.pseudo, taille, 0); //reçoit le message

    pthread_mutex_lock(&M1); //bloque l'accès au tableau
    int i = 0;
    while(tabdSC[i] != -1){ //si on trouve un slot de libre (qui existe forcément par la vérification fait au préalable)
        i = (i + 1)%NB_MAX_PERSONNE;
    }
    tabdSC[i] = dSC;

    args.id = i; //la position du socket du client dans le tableau
    args.dSC = dSC;
    pthread_mutex_unlock(&M1); //on redonne l'accès

    lecture_envoie(args); //le client va pouvoir commencer a communiquer
    pthread_exit(0);
}

//initialise la communication entre le client et le serveur
//Entrée : le socket d'écoute
//Sortie : renvoie rien, lance la fonction du pseudo avec le socket créé si la communication peut s'opérer
void init_connexion(int dS) {

    for(int i = 0; i<NB_MAX_PERSONNE;i++) { //initialise le tableau
        tabdSC[i] = -1;
    }

    struct sockaddr_in aC ; //structure du socket
    socklen_t lg = sizeof(struct sockaddr_in) ;

    char msg[15] ="serveur plein";   

    while(true){ //continue a s'éxecuter 

        int dSC = accept(dS, (struct sockaddr*) &aC,&lg); //crée le socket client
        if (dSC == -1) { //gestion de l'erreur de accept
            printf("problème de connexion\n");
        }
        else {
            pthread_mutex_lock(&M2); //bloque l'accès au compteur du nombre de personne
            if (NB_PERSONNE_ACTUELLE < NB_MAX_PERSONNE-1){
                
                int a = 0;
                send(dSC, &a, sizeof(int), 0);

                printf("Client Connecté\n");            
                NB_PERSONNE_ACTUELLE++;

                pthread_mutex_unlock(&M2); //redonne l'accès au nombre de client connecté

                pthread_t thread;
                pthread_create(&thread, NULL, choixPseudo,(void*)&dSC); //on lance le thread du client
            }
            else {
                int a = 1;
                send(dSC, &a, sizeof(int), 0);
                shutdown(dSC,2); //fin du socket car il n'y a plus de place dans la communication
            }
        }
    }
}

void ArretForce(int n) {
    printf("Coupure du programme\n");
    pthread_mutex_lock(&M1);
    for (int i = 0;i<NB_MAX_PERSONNE+1;i++) {
        shutdown(tabdSC[i],2);
    }
    pthread_mutex_unlock(&M1);
    exit(0);
}

//main de la fonciton
int main(int argc, char *argv[]) { 

    printf("Début programme\n");

    signal(SIGINT, ArretForce); //Ajout du signal de fin ctrl+c

    int dS = init_ouverture_connexion(atoi(argv[1])); //on crée le socket de communication 

    init_connexion(dS); //lancement de la connexion des clients
    shutdown(dS,2); //fin du socket de communication
    printf("fin programme");
}