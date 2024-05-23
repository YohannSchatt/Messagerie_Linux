// BUT DU PROG : Ce programme, client.c, est un client TCP simple permettant d'échanger des messages avec un serveur distant

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <dirent.h>

#define MAX_FILE 200 //limite max de personne sur le serveur

pthread_t th_envoie; //création des 2 threads, celui qui va lire les messages reçu et celui qui envoie son message au serveur
pthread_t th_recept;

pthread_mutex_t M1 = PTHREAD_MUTEX_INITIALIZER; //création du mutex qui permet d'assurer l'exclusion mutuelle pour la variable continu

int d = -1; //variable qui stocke dS (quand il est défini) pour l'arrêt forcé du serveur
int PORT;
char* IP;


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

void finFichier(int dSF){
    shutdown(dSF,2);
    printf("fin fichier\n");
    pthread_exit(0);
}

int foundSpace(char* commande){
    int i = 0;
    bool found = false;
    while (i<strlen(commande) && !found) {
        if (commande[i] == ' ' || commande[i] == '\0'){
            found = true;
        }
        i++;
    }
    return i;
}

long foundTaille(char* path){
    FILE *fichier;
    long taille;
    fichier = fopen("nom_du_fichier", "rb");
    if (fichier == NULL) {
        printf("Impossible d'ouvrir le fichier.\n");
        return 0;
    }
    fseek(fichier, 0, SEEK_END);
    taille = ftell(fichier);
    fclose(fichier);
    printf("La taille du fichier est : %ld octets\n", taille);
    return taille;
}

char* getCommande(char* commande){
    int pos = foundSpace(commande);
    char* res = (char*)malloc(sizeof(char)*pos+1);
    bool stop = false;
    int i = 0;
    while(i<strlen(commande) && !stop){
        if (commande[i] == ' '){
            stop = true;
        }
        res[i] = commande[i];
        i++;
    }
    return res;
}

char* getPath(char* commande){
    int pos = foundSpace(commande);
    char* path = (char*)malloc(sizeof(char)*(strlen(commande)-pos+1));
    for(int i=0;i+pos<strlen(commande);i++){
        path[i] = commande[pos+i];
    }
    return path;
}

int findLastSlash(char* path){
    int i = strlen(path)-1;
    bool found = false;
    while (i<0 && !found) {
        if (path[i] == '/'){
            found = true;
        }
        i++;
    }
    return i;
}

char* getNameFile(char* path){
    int pos = findLastSlash(path);
    char* name = (char*)malloc(sizeof(char)*(strlen(path)-pos+1));
    for(int i = 0;pos+i<strlen(path);i++){
        name[i] = path[pos+i];
    }
    return name;
}

void* lecture_fichier(char* nameFile,int dSF){
    FILE* fic;
    char* file = "file/";
    char* path = (char*)malloc(sizeof(char)*(8+strlen(nameFile)));
    path[0] = '\0'; //pour éviter de modifier des parties de mémoire ou on a pas accès
    strcat(path,file);
    strcat(path,nameFile);
    printf("%s\n",path);
    fic = fopen(path,"rb");
    short int Taille_buf = 256;
    short int buffer[Taille_buf];
    short int i, nb_val_lues = Taille_buf;
    int err;
    if(fic==NULL) {
        printf("ouverture du fichier impossible !");
        finFichier(dSF);
    }
    int taille_nameFile = strlen(nameFile)+1;
    err = send(dSF,&taille_nameFile,sizeof(int), 0);
    if (err == 0 | err == -1){
        printf("erreur d'envoie de la taille du fichier");
        finFichier(dSF);
    }
    err = send(dSF,nameFile,sizeof(char)*(taille_nameFile), 0);
    if (err == 0 | err == -1){
        printf("erreur envoie du nom du fichier");
        finFichier(dSF);
    }
    while ( nb_val_lues == Taille_buf ){
        printf("je lis le fichier");
        nb_val_lues = fread(buffer, sizeof(short int), Taille_buf, fic);
        err = send (dSF,&nb_val_lues,sizeof(short int), 0);
        if (err == 0 | err == -1){
            printf("erreur pendant l'envoie des valeurs lu du fichier");
            finFichier(dSF);
        }
        printf("j'envoie le fichier");
        err = send(dSF,buffer,sizeof(short int),0);
        if (err == 0 | err == -1){
            printf("erreur pendant l'envoie du fichier");
            finFichier(dSF);
        }
    }
    finFichier(dSF);
}

char* Interface_choix_fichier() {
    struct dirent *entry;
    DIR *dp;
    char *filenames[MAX_FILE];
    dp = opendir("./file");
    if (dp == NULL) {
        perror("opendir");
    }
    else {
        int i = 0;
        while ((entry = readdir(dp))) {
            if (entry->d_name[0] != '.' && i < MAX_FILE) {
            filenames[i] = strdup(entry->d_name);
            printf("%d : %s\n", i,filenames[i]);
            i++;
            }
        }
        int file_count = i;
        char* msg = (char*)malloc(sizeof(char)*16);
        printf("\33[2K\r");
        printf("écrivez un entier : ");
        fgets(msg,16,stdin);
        if (atoi(msg) < file_count && atoi(msg) >= 0){
            char* nameFile = strdup(filenames[atoi(msg)]);
            for (int j = 0; j < file_count; j++) {
                free(filenames[j]);
            }
            return nameFile;
        }
        else {
            printf("Choix invalide.\n");
            for (int j = 0; j < file_count; j++) {
                free(filenames[j]);
            }
            closedir(dp);
            return NULL;
        }
    }
}

void* thread_fichier(void* args_thread) {
    char* nameFile = Interface_choix_fichier();
    char* path = (char*)args_thread;
    int dSF = socket(PF_INET, SOCK_STREAM, 0); //crée le socket
    struct sockaddr_in aS;
    aS.sin_family = AF_INET;
    inet_pton(AF_INET,IP,&(aS.sin_addr)); 
    aS.sin_port = htons(PORT+1) ;
    socklen_t lgA = sizeof(struct sockaddr_in) ;
    if (connect(dSF, (struct sockaddr *) &aS, lgA) == -1) { //se connecte au serveur
        shutdown(dSF,2);
        fprintf(stderr, "Erreur lors de la création de la connexion\n");
        pthread_exit(0); //fin du programme
    }
    printf("coucou\n");
    lecture_fichier(nameFile,dSF);
    pthread_exit(0);   
}

//Fonction de lecture des messages et affiche les messages reçu des autres clients
//Entrée : le socket du serveur, le booléen qui gère l'exécution des deux threads
//Sortie : renvoie rien, affiche le message
bool lecture(int dS, bool* continu,char* pseudo){ 
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
            //pthread_mutex_lock(&M1); //on bloque l'accès au booléen car il peut être changé pendant la lecture
            if (*continu){
                printf("\33[2K\r");
                puts(msg);
                printf("%s : ", pseudo);
                setbuf(stdout, NULL);
            }
            //pthread_mutex_unlock(&M1); //on réouvre l'accès
        }
        free(msg);
    }
    return res;
}

//Fonction qui permet l'envoie des messages par l'utlisateur
//Entrée : le socket du serveur, le message
//Sortie : un booléen si il reçoit "fin" alors false, sinon true 
bool envoie(int dS, char** msg,bool* continu, char* pseudo){
    bool res = true;
    //pthread_mutex_lock(&M1);
    if (*continu){
        //pthread_mutex_unlock(&M1);
        fgets(*msg,128,stdin);
        char *pos = strchr(*msg,'\n'); //cherche le '\n' 
        *pos = '\0'; // le change en '\0' pour la fin du message et la cohérence de l'affichage
        if(strcmp(*msg,"/quitter") == 0 || strcmp(*msg,"/fermeture") == 0){
            res = false;
        }
        if (*msg[0] != '\0'){
            int taille = strlen(*msg)+1; //on récupère la taille du message (+1 pour le caractère de '\0')
            if (send(dS, &taille, sizeof(int), 0) == -1 || send(dS, *msg, taille, 0) == -1){ //envoie de la taille et le message
                res = false;
            }
        }
        if(strcmp(getCommande(*msg),"/sendFile") == 0){
            pthread_t th_file;
            printf("\33[2K\r");
            if (pthread_create(&th_file, NULL, thread_fichier, NULL) == -1) { //lance le thread propagation
                fprintf(stderr, "Erreur lors de la création du thread d'envoie\n");
            }
        }
        sleep(20); 
        printf("%s : ", pseudo); //affichage en dessous comme le message de join va permettre d'afficher avant
        return res;
    }
    return false;
}

//Fonction qui permet de réceptionner tout les messages
//Entrée : une structure qui sert d'argument pour que la fonction passe dans le thread et reçoit les données qui lui sont utiles
//Sortie : renvoie rien, gère la reception des messages
void* reception(void* args_thread) {
    struct Args_Thread args = *((struct Args_Thread*)args_thread); //récupère les arguments
    bool continu = *(args.continu);
    while(continu){ 
        //pthread_mutex_lock(&M1); //bloque l'accès au booléen (car peut être écrit pendant sa lecture)
        continu = *(args.continu);
        //pthread_mutex_unlock(&M1);  //redonne l'accès au booléen
        continu = lecture(args.dS,args.continu,args.pseudo);
    }
    //pthread_mutex_lock(&M1); //si fin de la communication alors on change le booléen donc on ferme l'accès au booléen le temps de l'affectation de false
    *args.continu = false;
    //pthread_mutex_unlock(&M1); //on redonne l'accès
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
        //pthread_mutex_lock(&M1); //bloque l'accès au booléen (car peut être écrit pendant sa lecture)
        continu = *(args.continu); //met à jour le booléen si modifié par la fonction propagation
        //pthread_mutex_unlock(&M1);  //redonne l'accès au booléen
        continu = envoie(args.dS,&msg,args.continu,args.pseudo);
    }
    //pthread_mutex_lock(&M1); //si fin de la communication alors on change le booléen donc on ferme l'accès au booléen le temps de l'affectation de false
    *args.continu = false; 
    //pthread_mutex_unlock(&M1); //on redonne l'accès
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
int main(int argc, char* argv[]){

    signal(SIGINT, ArretForce);
    IP = argv[1];
    PORT = atoi(argv[2]);

    if (argc != 3) { //si le programme n'a pas 2 arguments
        printf("./client IP Port\n");
    }
    else{
        printf("Début programme\n");
        int dS = socket(PF_INET, SOCK_STREAM, 0); //crée le socket
        d = dS;
        printf("Socket Créé\n");
        struct sockaddr_in aS;
        aS.sin_family = AF_INET;
        inet_pton(AF_INET,argv[1],&(aS.sin_addr)) ; 
        aS.sin_port = htons(atoi(argv[2])) ;
        socklen_t lgA = sizeof(struct sockaddr_in) ;
        if (connect(dS, (struct sockaddr *) &aS, lgA) == -1) { //se connecte au serveur
            shutdown(dS,2);
            fprintf(stderr, "Erreur lors de la création de la connexion\n");
            return EXIT_FAILURE; //fin du programme
        }
        else {
            int accept;
            printf("vous avez rejoint la file d'attente\n");
            int err = recv(dS, &accept, sizeof(bool), 0);
            if(err == -1 || err == 0){
                ArretForce(0);
            }
            printf("Vous êtes connecté au serveur !\n");
            bool* continu = (bool*)malloc(sizeof(bool)); //booléen utilisé dans les deux threads
            *continu = true;

            struct Args_Thread args_recept;
            args_recept.dS = dS;
            args_recept.continu = continu;

            struct Args_Thread args_envoie;
            args_envoie.dS = dS;
            args_envoie.continu = continu;

            // Les deux structures sont différents pour éviter des problèmes de concurrence même si ils prennent les même données 
            
            args_recept.pseudo = choixPseudo(dS); //l'utilisateur donne son pseudo
            args_envoie.pseudo = args_recept.pseudo;

            if (pthread_create(&th_recept, NULL, reception, (void*)&args_recept) == -1) { //lance le thread de réception
                shutdown(dS,2);
                fprintf(stderr, "Erreur lors de la création du thread de reception\n"); 
                return EXIT_FAILURE; //fin du programme
            }
            if (pthread_create(&th_envoie, NULL, propagation, (void*)&args_envoie) == -1) { //lance le thread propagation
                shutdown(dS,2);
                fprintf(stderr, "Erreur lors de la création du thread d'envoie\n");
                return EXIT_FAILURE; //fin du programme
            }
                
            pthread_join(th_recept,NULL); //attend la fin du thread de réception    
        }
        shutdown(dS,2);//met fin au socket

        setbuf(stdout, NULL);
        printf("\33[2K\r");
        printf("fin du programme\n");
    }
    return 0;
}