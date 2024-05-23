#include <stdio.h>

int main() {
    FILE *fichier;
    char ligne[256]; // Taille maximale d'une ligne du manuel
    fichier = fopen("manuel.txt", "r");
    if (fichier == NULL) {
        // Gérer l'erreur si le fichier n'a pas pu être ouvert
        printf("Erreur : Impossible d'ouvrir le fichier manuel.txt\n");
    } else {
        // Afficher le contenu du fichier ligne par ligne
        while (fgets(ligne, sizeof(ligne), fichier) != NULL) {
            printf("%s", ligne); // Affichage de la ligne à la console
        }
        fclose(fichier);
    }
    return 0;
}
