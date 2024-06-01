#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_FORBIDDEN_WORDS 100

// Liste des mots interdits
const char* forbiddenWords[MAX_FORBIDDEN_WORDS];
int forbiddenWordsCount = 0;

// Liste des mots "jolis"
const char* joliWords[MAX_FORBIDDEN_WORDS];
int joliWordsCount = 0;

// Fonction pour censurer les insultes dans le message
// Fonction pour censurer les insultes dans le message
// Fonction pour censurer les insultes dans le message
// Fonction pour censurer les insultes dans le message
void censorMessage(char* message) {
    char* token;
    char* rest = message;
    const char delimiters[] = " ,.-;:\t\n"; // Délimiteurs possibles entre les mots

    // Créer un tableau pour stocker les mots censurés avec des mots "jolis"
    char censoredMessage[256] = ""; // Assez grand pour stocker le message complet
    bool firstWord = true; // Indique si c'est le premier mot du message

    // Parcourir chaque mot dans le message
    while ((token = strtok_r(rest, delimiters, &rest))) {
        bool isForbidden = false;
        for (int i = 0; i < forbiddenWordsCount; i++) {
            if (strcmp(token, forbiddenWords[i]) == 0) {
                if (!firstWord) {
                    strcat(censoredMessage, " "); // Ajouter un espace avant le mot censuré
                }
                strcat(censoredMessage, joliWords[i]); // Ajouter le mot "joli" censuré
                isForbidden = true;
                break; // Passer au mot suivant une fois remplacé
            }
        }
        if (!isForbidden) {
            // Si le mot n'est pas interdit, l'ajouter tel quel
            if (!firstWord) {
                strcat(censoredMessage, " "); // Ajouter un espace avant le mot
            }
            strcat(censoredMessage, token); // Ajouter le mot au message censuré
        }
        firstWord = false;
    }

    // Copier le message censuré dans la chaîne d'origine
    strcpy(message, censoredMessage);
}




// Fonction pour charger les mots interdits à partir d'un fichier
void loadForbiddenWords(const char* filename, const char** words, int maxWords) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Erreur lors de l'ouverture du fichier %s\n", filename);
        exit(EXIT_FAILURE);
    }

    int count = 0;
    char line[100];

    while (fgets(line, sizeof(line), file)) {
        // Supprimer le saut de ligne à la fin
        line[strcspn(line, "\n")] = '\0';
        words[count] = strdup(line);
        count++;

        if (count >= maxWords) {
            fprintf(stderr, "Trop de mots interdits dans le fichier %s\n", filename);
            exit(EXIT_FAILURE);
        }
    }

    fclose(file);
    forbiddenWordsCount = count;
}

// Fonction pour charger les mots "jolis" à partir d'un fichier
void loadJoliWords(const char* filename, const char** words, int maxWords) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Erreur lors de l'ouverture du fichier %s\n", filename);
        exit(EXIT_FAILURE);
    }

    int count = 0;
    char line[100];

    while (fgets(line, sizeof(line), file)) {
        // Supprimer le saut de ligne à la fin
        line[strcspn(line, "\n")] = '\0';
        words[count] = strdup(line);
        count++;

        if (count >= maxWords) {
            fprintf(stderr, "Trop de mots jolis dans le fichier %s\n", filename);
            exit(EXIT_FAILURE);
        }
    }

    fclose(file);
    joliWordsCount = count;
}

// Fonction pour remplacer les mots interdits par des mots "jolis"
void replaceForbiddenWords(char* message) {
    for (int i = 0; i < forbiddenWordsCount; i++) {
        char* pos = strstr(message, forbiddenWords[i]);
        while (pos != NULL) {
            // Remplacer le mot interdit par un mot "joli"
            strcpy(pos, joliWords[i]);
            pos = strstr(pos + strlen(joliWords[i]), forbiddenWords[i]); // Recherche de la prochaine occurrence
        }
    }
}