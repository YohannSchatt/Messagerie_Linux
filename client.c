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
#include "file.c"
#include "censure.c"

#define MAX_FILE 200 //limite max de personne sur le serveur

pthread_t th_envoie; //création des 2 threads, celui qui va lire les messages reçu et celui qui envoie son message au serveur
pthread_t th_recept;

pthread_mutex_t M1 = PTHREAD_MUTEX_INITIALIZER; //création du mutex qui permet d'assurer l'exclusion mutuelle pour la variable continu

int d = -1; //variable qui stocke dS (quand il est défini) pour l'arrêt forcé du serveur
int PORT;
char* IP;

bool continuToSend = true; //variable qui bloque l'envoie des messages aux autres clients


struct Args_Thread { //structure permettant de transférer les arguments dans les différents threads
    int dS; //le socket du serveur
    bool* continu; //le booléen qui permet de stopper les 2 threads quand l'utilisateur ferme la connexion
    char* pseudo;
};

struct args_fichier {
    int dSF;
    int a; //0 pour sendFile, 1 pour getFile
};

/**
 * @brief fonction qui arrête le programme en coupant la communication
 * @param n nécessaire pour l'ajouter au signal mais inutile ici
*/
void ArretForce(int n) {
    printf("\33[2K\r");
    printf("Coupure du programme\n");
    if (d != -1){
        shutdown(d,2);
    }
    exit(0);
}

/**
 * @brief fonction qui permet de choisir un fichier dans file_client et lance l'envoie du fichier
 * @return renvoie le nom du fichier choisi
*/
char* Interface_choix_fichier_sendFile() {
        int file_count;
        char** filenames = getFileInFolder("./file_client",&file_count);
        for(int i = 0; i<file_count;i++){
            printf("%d : %s\n",i,filenames[i]);
        }
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
            return NULL;
        }
    }

/**
 * @brief fonction qui permet de choisir un fichier dans file_client et lance la réception du fichier
 * @return renvoie le nom du fichier choisi
*/
void Interface_choix_fichier_getFile(int dSF) {
    int file_count;
    int err = recv(dSF,&file_count,sizeof(int),0);
    if (err <= 0){
        printf("Erreur reception nombre de fichier !");
        continuToSend = true;
        finFichier(dSF);
    }
    int taille_msg;
    for(int i = 0; i < file_count; i++){
        err = recv(dSF,&taille_msg,sizeof(int),0);
        if (err <= 0){
            printf("Erreur reception taille nom fichier !");
            continuToSend = true;
            finFichier(dSF);
        }
        char* msg = (char*)malloc(sizeof(char)*taille_msg);
        err = recv(dSF,msg,sizeof(char)*taille_msg,0);        
        if (err <= 0){
            printf("Erreur connexion fichier !");
            continuToSend = true;
            finFichier(dSF);
        }
        printf("%d : %s\n",i, msg);
        free(msg);
    }
    char* choix = (char*)malloc(sizeof(char)*16);
    printf("\33[2K\r");
    printf("écrivez un entier : ");
    fgets(choix,16,stdin);
    int res = atoi(choix);
    free(choix);
    if (res >= 0 && res < file_count){
        err = send(dSF,&res,sizeof(int),0);
        if (err <= 0){
            printf("Erreur connexion fichier !");
            continuToSend = true;
            finFichier(dSF);
        }
    }
    else{
        printf("Choix invalide.\n");
        continuToSend = true;
        finFichier(dSF);
    }
}

/**
 * @brief fonction qui permet de lancer getFile ou sendFile (démarrage du socket avec acceptation des clients)
 * @param args void* car fonction passé en paramètre d'un phtread_create mais est retransformé en int qui est soit 0, soit 1
*/
void* thread_fichier(void* args) {

    int choix = *((int*)args);
    int dSF = socket(PF_INET, SOCK_STREAM, 0); //crée le socket
    printf("%d\n",dSF);
    struct sockaddr_in aS;
    aS.sin_family = AF_INET;
    inet_pton(AF_INET,IP,&(aS.sin_addr)); 
    aS.sin_port = htons(PORT+1) ;
    socklen_t lgA = sizeof(struct sockaddr_in) ;
    if (connect(dSF, (struct sockaddr *) &aS, lgA) == -1) { //se connecte au serveur
        shutdown(dSF,2);
        continuToSend = true;
        fprintf(stderr, "Erreur lors de la création de la connexion\n");
        pthread_exit(0); //fin du programme
    }
    int b;
    int err = recv(dSF,&b, sizeof(int), 0); //Accusé de réception pour qu'on envoie pas les messages avant que le serveur soit connecté
    if (err == 0 || err == -1){
        shutdown(dSF,2);
        continuToSend = true;
        fprintf(stderr, "Erreur lors de la création de la connexion\n");
        pthread_exit(0);
    }
    err = send(dSF,&choix,sizeof(int),0);
    if (choix == 0){ //0 pour sendFile, 1 pour getFile
        char* nameFile = Interface_choix_fichier_sendFile();
        continuToSend = true;
        sendFichier(nameFile,"./file_client/",dSF);
    }
    else {
        Interface_choix_fichier_getFile(dSF);
        continuToSend = true;
        recvFichier(dSF,"./file_client/");
    } 
}

/**
 * @brief Fonction de lecture des messages et affiche les messages reçu des autres clients
 * @param dS le socket du serveur
 * @param continu le booléen qui gère l'exécution des deux threads
 * @param pseudo string contenant le pseudo de l'utilisateur
*/

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
            if (*continu){
                printf("\33[2K\r");
                printf("%s\n",msg);
                printf("%s : ", pseudo);
                setbuf(stdout, NULL);
            }
        }
        free(msg);
    }
    return res;
}

/**
 * @brief fonction qui permet l'envoie des messages par l'utilisateur
 * @param dS le socket du serveur
 * @param msg le message
 * @param continu le booléen qui permet l'éxecution des deux threads
 * @param pseudo le pseudo de l'utilisateur
 * @return renvoie un booléen, renvoie false si on écrit /quitter ou /fermeture
*/
bool envoie(int dS, char** msg,bool* continu, char* pseudo){
    bool res = true;
    //pthread_mutex_lock(&M1);
    if (*continu){
        //pthread_mutex_unlock(&M1);
        fgets(*msg,128,stdin);
        char *pos = strchr(*msg,'\n'); //cherche le '\n' 
        *pos = '\0'; // le change en '\0' pour la fin du message et la cohérence de l'affichage
        
        // Censure des insultes dans le message
        censorMessage(*msg);

        if(strcmp(*msg,"/quitter") == 0 || strcmp(*msg,"/fermeture") == 0){
            res = false;
        }
        if(strcmp(*msg,"/sendFile") == 0){
            pthread_t th_file;
            printf("\33[2K\r");
            int a = 0;
            continuToSend = false;
            if (pthread_create(&th_file, NULL, thread_fichier, (void*)(&a)) == -1) { //lance le thread propagation
                fprintf(stderr, "Erreur lors de la création du thread d'envoie\n");
            }
            *msg[0] = '\0';
        }
        if(strcmp(*msg,"/getFile") == 0){
            pthread_t th_file;
            printf("\33[2K\r");
            int a = 1;
            continuToSend = false;
            if (pthread_create(&th_file, NULL, thread_fichier, (void*)(&a)) == -1) { //lance le thread propagation
                fprintf(stderr, "Erreur lors de la création du thread d'envoie\n");
            }
            *msg[0] = '\0';
        }
        if (*msg[0] != '\0'){
            int taille = strlen(*msg)+1; //on récupère la taille du message (+1 pour le caractère de '\0')
            if (send(dS, &taille, sizeof(int), 0) <= 0 || send(dS, *msg, taille, 0) <= 0){ //envoie de la taille et le message
                res = false;
            }
        }
        printf("%s : ", pseudo); //affichage en dessous comme le message de join va permettre d'afficher avant
        return res;
    }
    return false;
}

/**
 * @brief Fonction qui permet de receptionner tout les messages
 * @param args_threads une structure donner en argument d'un pthread donc void* qui permet de récupérer le pseudo ou le dS
*/
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

/**
 * @brief Fonction qui envoie les messages
 * @param args_thread une structure qui sert d'argument pour que la fonction passe dans le thread et reçoit les donné qui lui sont utiles
*/
void* propagation(void* args_thread){
    sleep(1);
    char* msg = (char*)malloc(128*sizeof(char)); //alloue la taille du message (128 max car fgets à 128 max)
    struct Args_Thread args = *((struct Args_Thread*)args_thread); //récupère les arguments
    bool continu = true;
    while(continu){
        //pthread_mutex_lock(&M1); //bloque l'accès au booléen (car peut être écrit pendant sa lecture)
        continu = *(args.continu); //met à jour le booléen si modifié par la fonction propagation
        //pthread_mutex_unlock(&M1);  //redonne l'accès au booléen
        if(continuToSend){
            continu = envoie(args.dS,&msg,args.continu,args.pseudo);
        }
    }
    //pthread_mutex_lock(&M1); //si fin de la communication alors on change le booléen donc on ferme l'accès au booléen le temps de l'affectation de false
    *args.continu = false; 
    //pthread_mutex_unlock(&M1); //on redonne l'accès
    free(msg);
    pthread_exit(EXIT_SUCCESS);
}
/**
 * @brief Fonction qui permet a l'utilissateur de sélectionner son pseudo
 * @param dS le socket du serveur
*/
char* choixPseudo(int dS){
    char* msg = (char*)malloc(16*sizeof(char)); //le message alloué a 16 max (taille du pseudo autorisé)
    bool continu = true;

    while(continu){
        printf("Choix de votre pseudo : "); 
        setbuf(stdout, NULL);
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
/**
 * @brief fonction principale du programme, pour lancer le programme il faut écrite : ./client "IP" "Port"
*/
int main(int argc, char* argv[]){

    signal(SIGINT, ArretForce);
    IP = argv[1];
    PORT = atoi(argv[2]);

    // Charger les mots "jolis" à partir du fichier spécifié
    loadJoliWords("./jolis_mots.txt", joliWords, MAX_FORBIDDEN_WORDS);

    loadForbiddenWords("./mots_interdits.txt", forbiddenWords, MAX_FORBIDDEN_WORDS);

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