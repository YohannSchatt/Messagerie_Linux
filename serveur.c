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

void fin_connexion(int dSC) {
    if (shutdown(dSC, 2) == -1) {
        handle_error("shutdown");
    }
    printf("Fermeture\n");
}

bool lecture(int dSC, char **msg) {
    bool res = true;
    int taille;
    if (recv(dSC, &taille, sizeof(int), 0) == -1) {
        handle_error("recv");
    }
    if (recv(dSC, *msg, taille, 0) == -1) {
        handle_error("recv");
    }
    if ((strcmp(*msg, "fin") == 0)) {
        res = false;
    }
    printf("%s\n", *msg);
    return res;
}

void envoie(int dSC, char **msg) {
    int taille = strlen(*msg) + 1;
    if (send(dSC, &taille, sizeof(int), 0) == -1) {
        handle_error("send");
    }
    if (send(dSC, *msg, taille, 0) == -1) {
        handle_error("send");
    }
}

int init_ouverture_connexion(int port) {
    int dS = socket(PF_INET, SOCK_STREAM, 0);
    if (dS == -1) {
        handle_error("socket");
    }
    printf("Socket Créé\n");

    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port = htons(port);
    if (bind(dS, (struct sockaddr *)&ad, sizeof(ad)) == -1) {
        handle_error("bind");
    }
    printf("Socket Nommé\n");

    if (listen(dS, 7) == -1) {
        handle_error("listen");
    }
    printf("Mode écoute\n");

    return dS;
}

int *init_connexion(int dS) {
    printf("Je suis dans init_connexion\n");

    struct sockaddr_in aC;
    socklen_t lg1 = sizeof(struct sockaddr_in);
    int dSC1 = accept(dS, (struct sockaddr *)&aC, &lg1);
    if (dSC1 == -1) {
        handle_error("accept");
    }
    printf("Client 1 Connecté\n");

    struct sockaddr_in aB;
    socklen_t lg2 = sizeof(struct sockaddr_in);
    int dSC2 = accept(dS, (struct sockaddr *)&aB, &lg2);
    if (dSC2 == -1) {
        handle_error("accept");
    }
    printf("Client 2 Connecté\n");

    int r1 = 0;
    int r2 = 1;

    if (send(dSC1, &r1, sizeof(int), 0) == -1) {
        handle_error("send");
    }

    if (send(dSC2, &r2, sizeof(int), 0) == -1) {
        handle_error("send");
    }

    int *tabdSC = malloc(2 * sizeof(int));
    tabdSC[0] = dSC1;
    tabdSC[1] = dSC2;
    return tabdSC;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Début programme\n");

    int dS = init_ouverture_connexion(atoi(argv[1]));

    char *msg = malloc(128 * sizeof(char));

    while (true) {
        int *tabdSC = init_connexion(dS);

        bool continu = true;
        int pos = 1;

        while (continu) {
            continu = lecture(tabdSC[pos], &msg);
            envoie(tabdSC[(pos + 1) % 2], &msg);
            pos = (pos + 1) % 2;
        }
        fin_connexion(tabdSC[0]);
        fin_connexion(tabdSC[1]);
        free(tabdSC);
    }
    free(msg);
    if (shutdown(dS, 2) == -1) {
        handle_error("shutdown");
    }
    printf("Fin programme\n");
}
