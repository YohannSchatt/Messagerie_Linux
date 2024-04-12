//BUT DU PROG : Ce programme est un serveur de chat qui accepte les connexions de deux clients simultanément et facilite la communication entre eux.
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

// Structure pour les arguments du thread
struct Args_Thread {
    int dSC;                // Descripteur de socket pour la connexion cliente
    int *dSC_autre;         // Descripteurs de socket pour les deux clients
    bool *continu;          // Pointeur vers un booléen pour indiquer si la communication continue
    char *msg;              // Tampon de message
};

// Fonction pour fermer une connexion
void fin_connexion(int dSC) {
    shutdown(dSC, 2);
    printf("Fermeture de la connexion\n");
}

// Fonction pour lire un message
bool lecture(int dSC, char **msg) {
    bool res = true;
    int taille;
    recv(dSC, &taille, sizeof(int), 0);   // Réception de la taille du message
    recv(dSC, *msg, taille, 0);            // Réception du message
    if ((strcmp(*msg, "fin") == 0)) {      // Vérification si le message est "fin"
        res = false;
    }
    printf("%s\n", *msg);                   // Affichage du message
    return res;
}

// Fonction pour envoyer un message
void envoie(int dSC, char **msg) {
    int taille = strlen(*msg) + 1;
    send(dSC, &taille, sizeof(int), 0);    // Envoi de la taille du message
    send(dSC, *msg, taille, 0);            // Envoi du message
}

// Fonction pour initialiser l'ouverture de la connexion
int init_ouverture_connexion(int port) {
    int dS = socket(PF_INET, SOCK_STREAM, 0);   // Création du socket
    printf("Socket créé\n");

    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port = htons(port);
    bind(dS, (struct sockaddr*) &ad, sizeof(ad));   // Nommer le socket
    printf("Socket nommé\n");

    listen(dS, 7);   // Mettre le socket en mode écoute
    printf("Mode écoute\n");

    return dS;
}

// Fonction pour initialiser la connexion avec deux clients
int* init_connexion(int dS) {
    printf("Je suis dans init_connexion\n");

    struct sockaddr_in aC;
    socklen_t lg1 = sizeof(struct sockaddr_in);
    int dSC1 = accept(dS, (struct sockaddr*) &aC, &lg1);   // Accepter la connexion du client 1
    printf("Client 1 connecté\n");

    struct sockaddr_in aB;
    socklen_t lg2 = sizeof(struct sockaddr_in);
    int dSC2 = accept(dS, (struct sockaddr*) &aB, &lg2);   // Accepter la connexion du client 2
    printf("Client 2 connecté\n");

    int r1 = 0;
    int r2 = 1;

    send(dSC1, &r1, sizeof(int), 0);   // Envoyer un indicateur au client 1
    send(dSC2, &r2, sizeof(int), 0);   // Envoyer un indicateur au client 2

    int* tabdSC = malloc(2 * sizeof(int));
    tabdSC[0] = dSC1;
    tabdSC[1] = dSC2;
    return tabdSC;
}

// Fonction pour la communication entre les clients
void communication(void* args_thread) {
    struct Args_Thread* args = (struct Args_Thread*) args_thread;
    while (*(args->continu)) {
        *(args->continu) = lecture(args->dSC, &(args->msg));
        envoie(*(args->dSC_autre), &(args->msg));
    }
}

//-----------------------------------------MAIN-------------------------------------------------

int main(int argc, char *argv[]) {
    printf("Début programme\n");

    int dS = init_ouverture_connexion(atoi(argv[1]));   // Initialiser l'ouverture de la connexion

    char* msg = malloc(128 * sizeof(char));   // Tampon pour les messages

    while (true) {
        int* tabdSC = init_connexion(dS);   // Initialiser la connexion avec deux clients

        bool continu = true;

        while (continu) {
            struct Args_Thread args_thread;
            args_thread.dSC = tabdSC[0];   // Descripteur de socket du premier client
            args_thread.dSC_autre = &(tabdSC[1]);   // Descripteur de socket du deuxième client
            args_thread.continu = &continu;   // Pointeur vers le booléen continu
            args_thread.msg = msg;   // Tampon de message

            pthread_t th_communication;
            pthread_create(&th_communication, NULL, communication, (void*) &args_thread);   // Démarrer le thread de communication

            continu = lecture(tabdSC[1], &msg);   // Lire le message du deuxième client
            envoie(tabdSC[0], &msg);   // Envoyer le message au premier client

            pthread_join(th_communication, NULL);   // Attendre la fin du thread de communication
        }

        fin_connexion(tabdSC[0]);   // Fermer la connexion avec le premier client
        fin_connexion(tabdSC[1]);   // Fermer la connexion avec le deuxième client
        free(tabdSC);   // Libérer la mémoire allouée pour le tableau de descripteurs de socket
    }

    free(msg);   // Libérer la mémoire allouée pour le tampon de message
    shutdown(dS, 2);   // Fermer le socket du serveur
    printf("Fin programme\n");
    return 0;
}
