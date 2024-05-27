#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <dirent.h>

#define TAILLE_BUF 256 //taille du buffer pour l'envoie du fichier
#define MAX_FILE 200 //limite max de personne sur le serveur

//Fonction qui met fin a la communication et au thread qui a utilisé cette fonction
void finFichier(int dSF){
    shutdown(dSF,2);
    pthread_exit(0);
}

//Permet d'obtenir tout les noms de fichier dans un directory
char** getFileInFolder(char* file, int* file_count){
    struct dirent *entry;
    DIR *dp;
    char** filenames = malloc(sizeof(char*)*MAX_FILE);
    dp = opendir(file);
    if (dp == NULL) {
        perror("opendir");
    }
    else {
        int i = 0;
        while ((entry = readdir(dp))) {
            if (entry->d_name[0] != '.' && i < MAX_FILE) {
            filenames[i] = strdup(entry->d_name);
            i++;
            }
        }
        *file_count = i;
        closedir(dp);
        return filenames;
    }
}

//Permet de créer le chemin pour que le programme sait ou trouver le fichier a envoyer
char* getPath(char* file,char* name){
    char* path = (char*)malloc(sizeof(char)*(strlen(file)+strlen(name)+1));
    path[0] = '\0'; //pour éviter de modifier des parties de mémoire ou on a pas accès
    strcat(path,file);
    strcat(path,name);
    printf("%s\n",path);
    return path;
}

//Fonction qui reçoit et crée le fichier dans le file passé en paramètre 
void recvFichier(int dSF,char* file){
    printf("%d\n",dSF);
    int taille_name;
    int err = recv(dSF,&taille_name,sizeof(int),0);
    if (err <= 0) {
        fprintf(stderr,"problème de reception de la taille du nom du fichier\n");
        finFichier(dSF); 
    }
    char* name = (char*)malloc(sizeof(char)*taille_name);
    err = recv(dSF,name,sizeof(char)*(taille_name),0);
    if (err == 0 || err == -1) {
        fprintf(stderr,"problème du nom du fichier\n");
        finFichier(dSF); 
    }
    char* path = getPath(file,name);
    FILE* fic;
    fic = fopen(path,"wb");
    short int taille_fic_recu = 1; //initialisé a 1 pour qu'il rentre dans la boucle
    while( taille_fic_recu > 0 ) {
        err = recv(dSF,&taille_fic_recu,sizeof(short int),0);
        if (err <= 0){
            fprintf(stderr,"problème de reception de la taille d'un fichier\n");
            finFichier(dSF);
            fclose(fic); 
        }
        if (taille_fic_recu > 0){
            short int* buffer = (short int*)malloc(sizeof(short int) * taille_fic_recu);
            err = recv(dSF,buffer,sizeof(short int)*(taille_fic_recu),0);
            if (err <= 0) {
                fprintf(stderr,"problème de reception de la taille d'un fichier\n");
                finFichier(dSF);
                fclose(fic); 
            }
            fwrite(buffer, sizeof(short int),taille_fic_recu,fic);
            free(buffer);
        }
    }
    short int a = 1;
    err = send(dSF,&a,sizeof(short int),0); //bloque la fin du thread pour couper la connexion pour que tout les messages soit lu
    fclose(fic); 
    pthread_exit(0);
}

//Fonction qui envoie le fichier correspondant au nom et au directory donné en paramètre
void* sendFichier(char* nameFile,char* file,int dSF){
    printf("%d\n",dSF);
    FILE* fic;
    char* path = getPath(file,nameFile);
    fic = fopen(path,"rb");
    short int buffer[TAILLE_BUF];
    short int i, nb_val_lues = TAILLE_BUF;
    int err;
    if(fic==NULL) {
        printf("ouverture du fichier impossible !");
        fclose(fic); 
        finFichier(dSF);
    }
    int taille_nameFile = strlen(nameFile)+1;
    printf("%d\n",taille_nameFile);
    err = send(dSF,&taille_nameFile,sizeof(int), 0);
    if (err <= 0){
        printf("erreur d'envoie de la taille du fichier");
        fclose(fic); 
        finFichier(dSF);
    }
    err = send(dSF,nameFile,sizeof(char)*(taille_nameFile), 0);
    if (err <= 0){
        printf("erreur envoie du nom du fichier");
        fclose(fic); 
        finFichier(dSF);
    }
    while ( nb_val_lues == TAILLE_BUF ){
        nb_val_lues = fread(buffer, sizeof(short int), TAILLE_BUF, fic);
        err = send(dSF,&nb_val_lues,sizeof(short int), 0);
        if (err == 0 | err == -1){
            printf("erreur pendant l'envoie des valeurs lu du fichier");
            fclose(fic); 
            finFichier(dSF);
        }
        err = send(dSF,buffer,sizeof(short int)*nb_val_lues,0);
        if (err == 0 | err == -1){
            printf("erreur pendant l'envoie du fichier");
            fclose(fic); 
            finFichier(dSF);
        }
    }
    short int a = 0;
    err = send(dSF,&a,sizeof(short int),0);
    if (err <= 0){
        printf("erreur fin d'envoie");
        fclose(fic); 
        finFichier(dSF);
    }
    err = recv(dSF,&a,sizeof(short int),0); //bloque la fin du thread pour couper la connexion pour que tout les messages soit lu
    if (err <= 0){
        printf("erreur fin d'envoie");
        fclose(fic); 
        finFichier(dSF);
    }
    fclose(fic); 
    finFichier(dSF);
}
