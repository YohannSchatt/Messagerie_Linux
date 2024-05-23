#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>

#define MAX_FILE 100

pthread_t th_envoie; //création des 2 threads, celui qui va lire les messages reçu et celui qui envoie son message au serveur
pthread_t th_recept;

pthread_mutex_t M1 = PTHREAD_MUTEX_INITIALIZER; //création du mutex qui permet d'assurer l'exclusion mutuelle pour la variable continu

int d = -1; //variable qui stocke dS (quand il est défini) pour l'arrêt forcé du serveur

struct Args_Thread { //structure permettant de transférer les arguments dans les différents threads
    int dS; //le socket du serveur
    bool* continu; //le booléen qui permet de stopper les 2 threads quand l'utilisateur ferme la connexion
    char* pseudo;
};

//Fonction qui prend un paramètre un signal et qui stop le programme proprement
void ArretForce(int n) {
    printf("\33[2K\r");
    printf("Coupure du programme\n");
    if (d != -1){
        shutdown(d,2);
    }
    exit(0);
}

// Fonction pour demander la liste des fichiers disponibles sur le serveur
void requestFileList(int dS) {
    char* request = "/getFile";
    int requestSize = strlen(request) + 1;
    send(dS, &requestSize, sizeof(int), 0);
    send(dS, request, requestSize, 0);
}

// Fonction pour recevoir et afficher la liste des fichiers du serveur
void receiveFileList(int dS) {
    int fileSize;
    recv(dS, &fileSize, sizeof(int), 0);
    char* fileList = (char*)malloc(fileSize * sizeof(char));
    recv(dS, fileList, fileSize, 0);
    printf("Liste des fichiers disponibles sur le serveur :\n%s\n", fileList);
    free(fileList);
}

//Fonction de lecture des messages et affiche les messages reçu des autres clients
//Entrée : le socket du serveur, le booléen qui gère l'exécution des deux threads
//Sortie : renvoie rien, affiche le message
bool lecture(int dS, bool* continu, char* pseudo){
    bool res = true;
    int taille;
    int err = recv(dS,&taille, sizeof(int), 0); //reception de la taille du message
    if (err == 0 || err == -1) {
        res = false;
    }
    else {
        char* msg = (char*)malloc(taille*sizeof(char));
        err = recv(dS, msg,taille, 0); //reçoit le message
        if (err == 0 || err == -1){
            res = false;
        }
        else {
            pthread_mutex_lock(&M1); //on bloque l'accès au booléen car il peut être changé pendant la lecture
            if (*continu){
                printf("\33[2K\r");
                puts(msg);
                printf("%s : ", pseudo);
                setbuf(stdout, NULL);
            }
            pthread_mutex_unlock(&M1); //on réouvre l'accès
        }
        free(msg);
    }
    return res;
}

//Fonction qui permet l'envoie des messages par l'utlisateur
//Entrée : le socket du serveur, le message
//Sortie : un booléen si il reçoit "fin" alors false, sinon true
bool envoie(int dS, char** msg, bool* continu, char* pseudo) {
    bool res = true;
    pthread_mutex_lock(&M1);
    if (*continu) {
        pthread_mutex_unlock(&M1);
        fgets(*msg, 128, stdin);
        printf("%s : ", pseudo);
        char *pos = strchr(*msg, '\n');
        if (pos != NULL) {
            *pos = '\0';
        }

        if (strcmp(*msg, "/quitter") == 0 || strcmp(*msg, "/fermeture") == 0) {
            res = false;
        } else if (strcmp(*msg, "/getFile") == 0) {
            requestFileList(dS);
            receiveFileList(dS);
        } else {
            int taille = strlen(*msg) + 1;
            if (send(dS, &taille, sizeof(int), 0) == -1 || send(dS, *msg, taille, 0) == -1) {
                res = false;
            }
        }
        return res;
    }
    pthread_mutex_unlock(&M1);
    return false;
}

//Fonction qui permet de réceptionner tout les messages
//Entrée : une structure qui sert d'argument pour que la fonction passe dans le thread et reçoit les données qui lui sont utiles
//Sortie : renvoie rien, gère la reception des messages
void* reception(void* args_thread) {
    struct Args_Thread args = *((struct Args_Thread*)args_thread); //récupère les arguments
    bool continu = *(args.continu);
    while(continu){
        pthread_mutex_lock(&M1); //bloque l'accès au booléen (car peut être écrit pendant sa lecture)
        continu = *(args.continu);
        pthread_mutex_unlock(&M1);  //redonne l'accès au booléen
        continu = lecture(args.dS,args.continu,args.pseudo);
    }
    pthread_mutex_lock(&M1); //si fin de la communication alors on change le booléen donc on ferme l'accès au booléen le temps de l'affectation de false
    *args.continu = false;
    pthread_mutex_unlock(&M1); //on redonne l'accès
    pthread_exit(EXIT_SUCCESS);
}

//Fonction qui envoie les messages
//Entrée : une structure qui sert d'argument pour que la fonction passe dans le thread et reçoit les données qui lui sont utiles
//Sortie : renvoie rien, gère l'envoie des messages
void* propagation(void* args_thread){
    sleep(1);
    char* msg = (char*)malloc(128*sizeof(char)); //alloue la taille du message (128 max car fgets à 128 max)
    struct Args_Thread args = *((struct Args_Thread*)args_thread); //récupère les arguments
    bool continu = true;
    while(continu){
        pthread_mutex_lock(&M1); //bloque l'accès au booléen (car peut être écrit pendant sa lecture)
        continu = *(args.continu); //met à jour le booléen si modifié par la fonction propagation
        pthread_mutex_unlock(&M1);  //redonne l'accès au booléen
        continu = envoie(args.dS,&msg,args.continu,args.pseudo);
    }
    pthread_mutex_lock(&M1); //si fin de la communication alors on change le booléen donc on ferme l'accès au booléen le temps de l'affectation de false
    *args.continu = false;
    pthread_mutex_unlock(&M1); //on redonne l'accès
    free(msg);
    pthread_exit(EXIT_SUCCESS);
}

//Fonction qui permet a l'utilisateur de sélectionner son pseudo
//Entrée : le socket du serveur
//Sortie : renvoie rien, envoie le pseudo au serveur
char* choixPseudo(int dS){
    char* msg = (char*)malloc(16*sizeof(char)); //le message alloué a 16 max (taille du pseudo autorisé)
    bool continu = true;

    while(continu){
        printf("Choix de votre pseudo : ");
        fgets(msg,16,stdin); //l'utilisateur écrit son pseudo
        char* pos = strchr(msg,'\n'); //cherche '\n' mis par défaut par fgets
        *pos = '\0';
        int taille = strlen(msg)+1; // +1 pour l'envoie de '\0'

        if(send(dS, &taille, sizeof(int), 0) == -1){ //envoie la taille
            ArretForce(0);
        }
        else {
            if (send(dS, msg, taille, 0) == -1) { //envoie le message
                ArretForce(0);
            }
        }
        int err = recv(dS, &continu, sizeof(bool), 0);
        if(err == -1 || err == 0){
            ArretForce(0);
        }
        if(continu){
            printf("Le Pseudo que vous avez choisi est déjà utilisé ou invalide\n");
        }
    }
    return msg;
}

//--------------------------------------main--------------------------------------------
//Fonction principale du programme
//pour lancer le programme il faut écrite : ./client "IP" "Port"
int main(int argc, char* argv[]) {
    signal(SIGINT, ArretForce);

    if (argc != 3) {
        printf("./client IP Port\n");
    } else {
        printf("Début programme\n");
        int dS = socket(PF_INET, SOCK_STREAM, 0);
        d = dS;
        printf("Socket Créé\n");
        struct sockaddr_in aS;
        aS.sin_family = AF_INET;
        inet_pton(AF_INET, argv[1], &(aS.sin_addr));
        aS.sin_port = htons(atoi(argv[2]));
        socklen_t lgA = sizeof(struct sockaddr_in);
        if (connect(dS, (struct sockaddr*)&aS, lgA) == -1) {
            shutdown(dS, 2);
            fprintf(stderr, "Erreur lors de la création de la connexion\n");
            return EXIT_FAILURE;
        } else {
            int accept;
            printf("Vous avez rejoint la file d'attente\n");
            int err = recv(dS, &accept, sizeof(int), 0);
            if (err <= 0) {
                ArretForce(0);
            }
            printf("Vous êtes connecté au serveur !\n");
            bool* continu = (bool*)malloc(sizeof(bool));
            *continu = true;

            struct Args_Thread args_recept;
            args_recept.dS = dS;
            args_recept.continu = continu;

            struct Args_Thread args_envoie;
            args_envoie.dS = dS;
            args_envoie.continu = continu;

            args_recept.pseudo = choixPseudo(dS);
            args_envoie.pseudo = args_recept.pseudo;

            if (pthread_create(&th_recept, NULL, reception, (void*)&args_recept) != 0) {
                shutdown(dS, 2);
                fprintf(stderr, "Erreur lors de la création du thread de réception\n");
                return EXIT_FAILURE;
            }
            if (pthread_create(&th_envoie, NULL, propagation, (void*)&args_envoie) != 0) {
                shutdown(dS, 2);
                fprintf(stderr, "Erreur lors de la création du thread d'envoi\n");
                return EXIT_FAILURE;
            }

            pthread_join(th_recept, NULL);
            pthread_join(th_envoie, NULL);

            shutdown(dS, 2);
            printf("\33[2K\rfin du programme\n");
        }
    }
    return 0;
}

