#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h> // Pour errno et perror

void handle_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void fin(int dS, char **msg) {
    if (shutdown(dS, 2) == -1) {
        handle_error("shutdown");
    }
    free(*msg);
    printf("Fin du programme\n");
}

bool lecture(int dS, char **msg) {
    bool res = true;
    int taille;
    if (recv(dS, &taille, sizeof(int), 0) == -1) {
        handle_error("recv");
    }
    if (recv(dS, *msg, taille, 0) == -1) {
        handle_error("recv");
    }
    if ((strcmp(*msg, "fin") == 0)) {
        res = false;
    }
    printf("L'autre utilisateur dit : %s\n", *msg);
    return res;
}

bool envoie(int dS, char **msg) {
    bool res = true;
    printf("Ecrit un message : ");
    fgets(*msg, 128, stdin);
    char *pos = strchr(*msg, '\n');
    *pos = '\0';
    if ((strcmp(*msg, "fin") == 0)) {
        res = false;
    }
    int taille = strlen(*msg) + 1;
    if (send(dS, &taille, sizeof(int), 0) == -1) {
        handle_error("send");
    }
    if (send(dS, *msg, taille, 0) == -1) {
        handle_error("send");
    }
    return res;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <IP> <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Début programme\n");
    int dS = socket(PF_INET, SOCK_STREAM, 0);
    if (dS == -1) {
        handle_error("socket");
    }
    printf("Socket Créé\n");

    struct sockaddr_in aS;
    aS.sin_family

