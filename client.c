// BUT DU PROG : Ce programme, client.c, est un client TCP simple permettant d'échanger des messages avec un serveur distant

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

pthread_t th_envoie, th_recept;

// Structure pour les arguments du thread
struct Args_Thread {
    int dS;             // Descripteur de socket
    bool *continu;      // Pointeur vers un booléen pour indiquer si la communication continue
    char *msg;          // Pointeur vers un tampon de message
};

// Fonction de fermeture de la connexion
void fin(int dS, char **msg) {
    shutdown(dS, 2);    // Fermeture du socket
    free(msg[0]);       // Libération de la mémoire allouée pour les messages
    free(msg[1]);
    free(msg);
    printf("Fin du programme");
}

// Fonction pour lire un message du serveur
bool lecture(int dS, char **msg) {
    bool res = true;
    int taille;
    recv(dS, &taille, sizeof(int), 0);    // Réception de la taille du message
    recv(dS, *msg, taille, 0);             // Réception du message
    if ((strcmp(*msg, "fin") == 0)) {      // Vérification si le message est "fin"
        res = false;
    }
    puts(*msg);                             // Affichage du message reçu
    return res;
}

// Fonction pour envoyer un message au serveur
bool envoie(int dS, char **msg) {
    bool res = true;
    printf("Ecrit un message : ");
    fgets(*msg, 128, stdin);                // Lecture du message depuis l'entrée standard
    char *pos = strchr(*msg, '\n');
    *pos = '\0';
    if ((strcmp(*msg, "fin") == 0)) {      // Vérification si le message est "fin"
        res = false;
    }
    int taille = strlen(*msg) + 1;
    send(dS, &taille, sizeof(int), 0);     // Envoi de la taille du message
    send(dS, *msg, taille, 0);             // Envoi du message
    return res;
}

// Fonction du thread pour la réception de messages du serveur
void *reception(void *args_thread) {
    struct Args_Thread *args = (struct Args_Thread *) args_thread;
    while (*(args->continu)) {
        *(args->continu) = lecture(args->dS, &(args->msg));
    }
    pthread_exit(0);
}

// Fonction du thread pour l'envoi de messages au serveur
void *propagation(void *args_thread) {
    struct Args_Thread *args = (struct Args_Thread *) args_thread;
    while (*(args->continu)) {
        *(args->continu) = envoie(args->dS, &(args->msg));
    }
    pthread_exit(0);
}

//-----------------------------------------MAIN-------------------------------------------------

int main(int argc, char *argv[]) {
    // Vérification des arguments de la ligne de commande
    if (argc != 3) {
        printf("./client IP Port");
    } else {
        printf("Début programme\n");

        // Création du socket
        int dS = socket(PF_INET, SOCK_STREAM, 0);
        printf("Socket Créé\n");

        // Configuration de l'adresse du serveur
        struct sockaddr_in aS;
        aS.sin_family = AF_INET;
        inet_pton(AF_INET, argv[1], &(aS.sin_addr));
        aS.sin_port = htons(atoi(argv[2]));
        socklen_t lgA = sizeof(struct sockaddr_in);
        connect(dS, (struct sockaddr *) &aS, lgA);
        printf("Socket Connecté\n");

        // Allocation de mémoire pour les variables de contrôle et les tampons de message
        bool *continu = (bool *) malloc(sizeof(bool));
        *continu = true;
        char *msg_envoie = malloc(128 * sizeof(char));
        char *msg_lecture = malloc(128 * sizeof(char));

        // Initialisation des arguments des threads
        struct Args_Thread *args_recept;
        args_recept->dS = dS;
        args_recept->continu = continu;
        args_recept->msg = msg_lecture;

        struct Args_Thread *args_envoie;
        args_envoie->dS = dS;
        args_envoie->continu = continu;
        args_envoie->msg = msg_envoie;

        // Création des threads pour la réception et l'envoi de messages
        pthread_create(&th_recept, NULL, reception, args_recept);
        pthread_create(&th_envoie, NULL, propagation, args_envoie);

        // Attente de la fin des threads
        pthread_join(th_recept, NULL);
        pthread_join(th_envoie, NULL);

        // Libération de la mémoire et fermeture de la connexion
        char **msg = (char **) malloc(2 * sizeof(char *));
        msg[0] = msg_envoie;
        msg[1] = msg_lecture;
        fin(dS, msg);
    }
    return 0;
}
