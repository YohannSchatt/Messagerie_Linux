#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>

pthread_t th_envoie,th_recept; //création des 2 threads, celui qui va lire les messages reçu et celui qui envoie son message au serveur

pthread_mutex_t M1 = PTHREAD_MUTEX_INITIALIZER; //création du mutex qui permet d'assurer l'exclusion mutuelle pour la variable continu

//
struct Args_Thread { //structure permettant de transférer les arguments dans les différents threads
    int dS; //le socket du serveur
    bool* continu; //le booléen qui permet de stopper les 2 threads quand l'utilisateur ferme la connexion
};

//mets fin au programme et ferme la communication avec le serveur
void fin(int dS) {
    shutdown(dS,2);
}

//Fonction de lecture des messages et affiche les messages reçu des autres clients
//Entrée : le socket du serveur, le booléen qui gère l'exécution des deux threads
//Sortie : renvoie rien, affiche le message
void lecture(int dS, bool* continu){ 
    bool res = true; //le booléen est initialisé a vrai
    int taille; //la taille du message
    recv(dS,&taille, sizeof(int), 0); //reception de la taille du message
    char* msg = (char*)malloc(taille*sizeof(char)); //on alloue l'espace nécessaire au message
    recv(dS, msg,taille, 0); //on reçoit le message
    pthread_mutex_lock(&M1); //on bloque l'accès au booléen car il peut être changé pendant la lecture
    if (*continu){ //si la communication n'est pas coupé 
        puts(msg); //affiche le message
    }
    pthread_mutex_unlock(&M1); //on réouvre l'accès
    free(msg); //on libère l'espace du message
}

//Fonction qui permet l'envoie des messages par l'utlisateur
//Entrée : le socket du serveur, le message
//Sortie : un booléen si il reçoit "fin" alors false, sinon true 
bool envoie(int dS, char** msg){
    bool res = true; //initialisé a true 
    fgets(*msg,128,stdin); //l'utilisateur écrit son message
    char *pos = strchr(*msg,'\n'); //cherche le '\n' 
    *pos = '\0'; // le change en '\0' pour la fin du message et la cohérence de l'affichage
    if(strcmp(*msg,"fin") == 0){ //compare le message reçu
        res = false; //si on reçoit fin le booléen est mis a false
    }
    int taille = strlen(*msg)+1; //on récupère la taille du message (+1 pour le caractère de fin de String)
    send(dS, &taille, sizeof(int), 0); //envoie de la taille
    send(dS, *msg, taille, 0); //envoie du message
    return res; //renvoie le booléen
}

//Fonction qui permet de réceptionner tout les messages
//Entrée : une structure qui sert d'argument pour que la fonction passe dans le thread et reçoit les données qui lui sont utiles
//Sortie : renvoie rien, gère la reception des messages
void* reception(void* args_thread) {
    struct Args_Thread args = *((struct Args_Thread*)args_thread); //récupère les arguments
    bool continu = *(args.continu); //récupère le booléen
    while(continu){ 
        lecture(args.dS,args.continu);
        pthread_mutex_lock(&M1); //bloque l'accès au booléen (car peut être écrit pendant sa lecture)
        continu = *(args.continu); //met à jour le booléen si modifié par la fonction propagation
        pthread_mutex_unlock(&M1);  //redonne l'accès au booléen
    }
    printf("j'ai reçu le fin dans reception");
    pthread_exit(0); //on ferme le thread
}
//Fonction qui envoie les messages 
//Entrée : une structure qui sert d'argument pour que la fonction passe dans le thread et reçoit les données qui lui sont utiles
//Sortie : renvoie rien, gère l'envoie des messages
void* propagation(void* args_thread){
    char* msg = (char*)malloc(128*sizeof(char)); //allou la taille du message (128 max car fgets a 128 max)
    struct Args_Thread args = *((struct Args_Thread*)args_thread); //récupère les arguments
    bool continu = true;
    while(continu){
        continu = envoie(args.dS,&msg); //fonction qui envoie le message
    }
    printf("j'ai reçu le fin dans propagation");
    pthread_mutex_lock(&M1); //si fin de la communication alors on change le booléen donc on ferme l'accès au booléen le temps de l'affectation de false
    *args.continu = false; //on le set a false 
    pthread_mutex_unlock(&M1); //on redonne l'accès
    free(msg); //on libère l'espace du message
    pthread_exit(0); //on ferme le thread
}

//Fonction qui permet a l'utilisateur de sélectionner son pseudo
//Entrée : le socket du serveur
//Sortie : renvoie rien, envoie le pseudo au serveur
void choixPseudo(int dS){
    char* msg = (char*)malloc(16*sizeof(char)); //le message alloué a 16 max (taille du pseudo autorisé)
    printf("Choix de votre pseudo : "); 
    fgets(msg,16,stdin); //l'utilisateur écrit son pseudo
    char* pos = strchr(msg,'\n'); //cherche '\n'
    *pos = '\0'; //le change en '\0'
    int taille = strlen(msg)+1; //récupère la nouvelle taille + ajoute 1 pour le caractère de fin de string
    send(dS, &taille, sizeof(int), 0); //envoie la taille
    send(dS, msg, taille, 0); //envoie le message
    free(msg); //libère la mémoire du message
}

void ArretForce(int n) {
    printf("Coupure du programme\n");
    exit(0);
}

//--------------------------------------main--------------------------------------------
//Fonction principale du programme
//pour lancer le programme il faut écrite : ./client "IP" "Port"
int main(int argc, char* argv[]){

    signal(SIGINT, ArretForce);

    if (argc != 3) { //si le programme n'a pas 2 arguments
        printf("./client IP Port\n");
    }
    else{
        printf("Début programme\n");
        int dS = socket(PF_INET, SOCK_STREAM, 0); //crée le socket
        printf("Socket Créé\n");
        struct sockaddr_in aS;
        aS.sin_family = AF_INET;
        inet_pton(AF_INET,argv[1],&(aS.sin_addr)) ; 
        aS.sin_port = htons(atoi(argv[2])) ;
        socklen_t lgA = sizeof(struct sockaddr_in) ;
        if (connect(dS, (struct sockaddr *) &aS, lgA) == -1) { //se connecte au serveur
            fprintf(stderr, "Erreur lors de la création de la connexion\n"); //si y a un problème 
            return EXIT_FAILURE; //fin du programme
        }
        else {
            printf("Socket Connecté\n");

            bool* continu = (bool*)malloc(sizeof(bool)); //alloue la place pour le pointeur de booléen utilisé dans les deux threads
            *continu = true; //assigne sa valeur

            struct Args_Thread args_recept; //structure des arguments de reception
            args_recept.dS = dS;
            args_recept.continu = continu;

            struct Args_Thread args_envoie; //structure des arguments de message
            args_envoie.dS = dS;
            args_envoie.continu = continu;

            // Les deux structures sont différents pour éviter des problèmes de concurrence même si ils prennent les même données 
    
            choixPseudo(dS); //l'utilisateur donne son pseudo

            if (pthread_create(&th_recept, NULL, reception, (void*)&args_recept) == -1) { //lance le thread de réception
                fprintf(stderr, "Erreur lors de la création du thread de reception\n"); //si y a un problème 
                return EXIT_FAILURE; //fin du programme
            }

            if (pthread_create(&th_envoie,NULL, propagation, (void*)&args_envoie) == -1) { //lance le thread propagation
                fprintf(stderr, "Erreur lors de la création du thread d'envoie\n"); //si y a un problème 
                return EXIT_FAILURE; //fin du programme
            }
            
            pthread_join(th_recept,NULL); //attend la fin du thread de réception
            pthread_join(th_envoie,NULL); //attend la fin du thread de propagation

            fin(dS); //met fin au socket

            printf("fin du programme\n");
        }
    }
}