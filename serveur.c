//BUT DU PROG : Ce programme est un serveur de chat qui accepte les connexions de deux clients simultanément et facilite la communication entre eux.
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>

#define NB_MAX_PERSONNE 100 //limite max de personne sur le serveur
int  NB_PERSONNE_ACTUELLE = 0;//compteur du nombre de personne connecté

pthread_mutex_t M1 = PTHREAD_MUTEX_INITIALIZER; //mutex qui protège l'accès au tableau des sockets clients
pthread_mutex_t M2 = PTHREAD_MUTEX_INITIALIZER; //mutex qui protège l'accès au nombre  des sockets clients

struct mem_Thread { //structure permettant de transférer les arguments dans les différents threads
    int id; //l'id pour retrouver les éléments dans les tableaux
    int dSC; //le socket du client
    char* pseudo; //son pseudo
};

struct Args_Thread {
    int id; //l'id pour retrouver les éléments dans les tableaux
    int dSC; //le socket du client
};

struct client {
    int dSC; //socket du client
    char* pseudo; //son pseudo
    pthread_t thread; //son thread
};

struct client tabdSC[NB_MAX_PERSONNE+1]; //tableau des sockets des clients

//Fonction qui prend en paramètre un socket et l'id dans le tableau
// elle va supprimer le client du tableau puis fermer le socket
//Entrée : le socket et l'id
//Sortie : tableau des sockets modifié
void fin_connexion(int dSC,int id) {

    pthread_mutex_lock(&M1); //empêche le tableau d'être accédé pour éviter problème d'exclusion mutuelle
    tabdSC[id].dSC = -1;
    pthread_mutex_unlock(&M1); //reouvre le tableau

    pthread_mutex_lock(&M2); //reouvre le nombre de personne
    NB_PERSONNE_ACTUELLE--;
    pthread_mutex_unlock(&M2); //empêche le nombre de personne présente d'être accédé pour éviter problème d'exclusion mutuelles

    shutdown(dSC,2); //ferme le socket
    printf("fermeture\n");
}

//Fonction qui reçoit un message du client associé au socket donnée en paramètre et le met dans message qui sera transmit a la fonction envoyer
//Entrée : le socket et le pointeur du message
//Sortie : un Booléen qui précise si on continu la communication, et met à jour le message
bool lecture(int dSC,char **msg){
    bool res = true;
    int taille;
    int err = recv(dSC,&taille, sizeof(int), 0);
    if (err != -1 && err != 0){ //communication de la taille
        *msg = (char*)malloc(taille*sizeof(char));
        err = recv(dSC,*msg, taille, 0);
        if (err != -1 && err != 0){ //reçoit le message
            if((strcmp(*msg,"fin") == 0)){ //si on reçoit fin alors on change le booléen pour couper la communication
                res = false;
            }
        }
        else {
            res = false;
        }
    }
    else {
        res = false;
    }
    return res;
}

//Fonction qui envoie le message donnée en paramètre avec le pseudo correspondant au client qu'on a en paramètre grâce au socket
//Entrée : le socket, le message, et le pseudo
//Sortie : renvoie rien, le message est envoyé
void envoie(int dSC,char* msg){
    int taille = strlen(msg)+1;
    if(send(dSC, &taille, sizeof(int), 0) != -1){
        send(dSC, msg, taille, 0);
    }
}

//Fonction qui envoie message donné en paramètre a tout le monde sauf le client qui a crée le message 
//Entrée : le socket du client qui envoie le message
//Sortie : renvoie rien, a envoyé le message à tout le monde sauf le client a l'origine du message
void envoie_everyone_client(int dSC,char* msg){
    for(int i = 0;i<NB_MAX_PERSONNE;i++) {
        pthread_mutex_lock(&M1); //on bloque l'accès au tableau
        if (tabdSC[i].dSC != -1 && dSC != tabdSC[i].dSC) { //si le socket existe et est différent de celui de notre client alors on envoie le message
            envoie(tabdSC[i].dSC, msg);
        }
        pthread_mutex_unlock(&M1); //on redonne l'accès au tableau
    }
}

//Fonction utilisé uniquement par le serveur qui envoie un message donné en paramètre a tout le monde 
//Entrée : un String (le message)
//Sortie : renvoie rien, envoie le message à tout le monde
void envoie_everyone_serveur(char* msg){
    for(int i = 0;i<NB_MAX_PERSONNE;i++) {
        pthread_mutex_lock(&M1); //on bloque l'accès au tableau
        if (tabdSC[i].dSC != -1) { //si le socket existe et est différent de celui de notre client alors on envoie le message
            envoie(tabdSC[i].dSC, msg);
        }
        pthread_mutex_unlock(&M1); //on redonne l'accès au tableau
    }
}

//Fonction qui envoie le message donné en paramètre 
//Entrée : un tableau de dSC qui envoie stocke les sockets des clients qui doivent recevoir le message
//Sortie : renvoie rien, le message est envoyé au personne dont le socket correspond 
// void envoie_prive(int* dSC, char* msg){
//     for(int i = 0;i<NB_MAX_PERSONNE;i++){
//         pthread_mutex_lock(&M1); //on bloque l'accès au tableau
//         if (tabdSC[i].dSC != -1 && args.dSC != tabdSC[i].dSC) { //si le socket existe et est différent de celui de notre client alors on envoie le message
//             envoie(tabdSC[i].dSC, msg);
//         }
//         pthread_mutex_unlock(&M1); //on redonne l'accès au tableau
//     }
// }

//Fonction qui permet de récupérer le pseudo dans une commande
//Entrée : l'adresse du msg de la commande, la position de l'espace juste devant le pseudo
//Sortie : le pseudo de l'utilisateur 
char* recup_pseudo(char* msg,int pos){
    char* pseudo = (char*)malloc(16*sizeof(char)); //taille max de 16 pour un pseudo
    int i = 0;
    while( i < 16 && msg[pos+i] != ' ' && msg[pos+i] != '\0'){
        pseudo[i] = msg[pos+i];
    }
    return pseudo;
}

//Fonction qui permet de récupérer le pseudo dans une commande 
//Entrée : l'adresse du msg de la commande, la position de l'espace juste devant le message
//Sortie : le message écrit par l'utilisateur 
char* recup_message(char* msg, int pos){
    int count = 0;
    while(msg[pos+count] != '\0'){
        count++;
    }
    char* message = (char*)malloc(count*sizeof(char));
    for(int i = 0; i < count ; i++){
        message[i] = msg[pos+i];
    }
    return message;
}

// int protocol(char *msg){
//     char pos = msg[0];
//     if (pos = '@'){
//         if verif_commande(msg,"everyone") {
//             envoie_all();
//         }
//         else if verif_commande(msg,"mp"){
//             recup_pseudo();
//             envoie_prive();
//         }
//     }
//     else if (pos = '/') {
//         if verif_commande(msg,"end") {
//             fin_thread();
//         }
//         else {
//             envoie_prive();
//         }
//     }
// }

//Fonction qui permet de vérifier la char* entrée par l'utilisateur existe
bool verif_commande(char* msg,char* msg_commande){
    bool res = true;
    int i = 0;
    if ((strlen(msg)-1) == (strlen(msg_commande))){ //msg a -1 car on a le lanceur de commande devant
            while (i<strlen(msg_commande) && msg[i] == '\0' && res ) {
            if (msg[i+1] == msg_commande[i]) { //i+1 car on ne regarde pas le lanceur de commande
                i++;
            }
            else {
                res = false;
            }
        }
    }
    return res;
}

//Cette fonction fusionne le pseudo de la personne concerné, et le message principale avec des caractères qui les joints
//Entrée : trois String (un pseudo, un message, et un la jointure)
//Sortie : renvoie l'adresse d'un String avec comme forme "pseudo jointure message"
char** creation_msg_serveur(char* msg, char* pseudo,char* jointure) {
    int taillemsg = strlen(msg); 
    int taillepseudo = strlen(pseudo);
    int taillejointure = strlen(jointure);
    char* message = (char*)malloc((taillemsg+taillepseudo+taillejointure+1)*sizeof(char)); //taille du msg (+1 pour '\0)
    char** adressemessage = (char**)malloc(sizeof(char*));
    *adressemessage = message;
    strcat(message,pseudo);
    strcat(message,jointure);
    strcat(message,msg);
    return adressemessage;
}

//Cette fonction fusionne le pseudo de l'utilisateur donné en paramètre et le message donné aussi en paramètre
//Entrée : deux String (un pseudo et un message)
//Sortie : renvoie l'adresse d'un String avec comme forme "pseudo : message"
char** creation_msg_client(char* msg, char* pseudo) {
    return creation_msg_serveur(msg,pseudo," : ");
}


//Cette fonction gère la lecture du message d'un client qui sera ensuite envoyé a tout les clients
//Entrée : une struct d'argument pour le thread, contenant le socket du client, et le pseudo et l'id dans le tableau des sockets client
//Sortie : renvoie rien, mais assure la liaison entre les clients tant que le client du socket ne coupe pas la communication
void lecture_envoie(struct mem_Thread args) {
    bool continu = true; //booléen qui va assurer la boucle tant que la communication n'est pas coupé 
    char* msgrecu = (char*)malloc(sizeof(char));
    msgrecu[0] ='\0'; 
    while (continu) {
        continu = lecture(args.dSC, &msgrecu);
        if (continu){
            char** msgcomplet = (char**)malloc(sizeof(char*));
            msgcomplet = creation_msg_client(msgrecu, args.pseudo);
            envoie_everyone_client(args.dSC,*msgcomplet);
        }
    }
    free(msgrecu);

    //message de fin de communication

    char** msgcomplet = (char**)malloc(sizeof(char*)); 
    msgcomplet = creation_msg_serveur("a quitté le serveur",args.pseudo," ");
    envoie_everyone_client(args.dSC,*msgcomplet);
    fin_connexion(args.dSC,args.id); //si communication coupé alors on mets fin au socket
}

//Fonction qui a pour but d'initialiser le socket de connexion initial
//Entrée : Le port donnée en argument du serveur quand on lance le programme
//Sortie : le socket inital de communication
int init_ouverture_connexion(int port) {
    
    int dS = socket(PF_INET, SOCK_STREAM, 0); //crée le socket en TCP
    if (dS == -1){
        fprintf(stderr,"erreur lors de la création du socket");
        exit(0);
    }
    else {
       printf("Socket Créé\n");
        struct sockaddr_in ad; // structure des sockets
        ad.sin_family = AF_INET;
        ad.sin_addr.s_addr = INADDR_ANY ;
        ad.sin_port = htons(port) ;
        if (bind(dS, (struct sockaddr*)&ad, sizeof(ad)) == -1) { //Donne un nom au socket
            shutdown(dS,2);
            fprintf(stderr,"problème de nommage du socket\n");
            exit(0);
        }
        else {
            printf("Socket Nommé\n");
            if(listen(dS, 7) == -1){  //mets en position d'écoute
                shutdown(dS,2);
                fprintf(stderr,"problème à initialiser l'écoute\n");
                exit(0);
            }
            else {
                printf("Mode écoute\n");
                return dS;
            }
        }
    }
}

bool verif_pseudo(char* pseudo){
    bool res = true;
    int i = 0;
    while (i<NB_MAX_PERSONNE && res){
        if(tabdSC[i].dSC != -1 && strcmp(tabdSC[i].pseudo,pseudo) == 0){
            res = false;
        }
        i++;
    }
    return res;
}

void* choixPseudo(void* args_thread){
    struct Args_Thread th = *((struct Args_Thread*)args_thread); //le socket client

    struct mem_Thread args; //crée la stucture permettant de mettre en des arguments a la fonction envoie_lecture

    int taille;

    bool continu = true;

    while(continu){
        recv(th.dSC,&taille, sizeof(int), 0); //reçoit la taille du message
        args.pseudo = (char*)malloc(taille*sizeof(char)); 
        recv(th.dSC, args.pseudo, taille, 0); //reçoit le message
        char* pos = (char*)malloc(sizeof(char));
        pos = strchr(args.pseudo,' ');
        if (strlen(args.pseudo)> 0 && args.pseudo[0] != ' '){ //vérifie si le premier caractère n'est pas un espace
            if (pos != NULL){ //propriété de la fonction strchr renvoie NULL si il n'y a pas le caratère demandé
                *pos = '\0';
            }
            if (verif_pseudo(args.pseudo)){
                continu = false;
            }
        }
        send(th.dSC,&continu, sizeof(bool), 0);
    }



    args.id = th.id; //la position du socket du client dans le tableau
    args.dSC = th.dSC;

    tabdSC[args.id].dSC = args.dSC;
    tabdSC[args.id].pseudo = args.pseudo; 

    char** msgcomplet = (char**)malloc(sizeof(char*));
    msgcomplet = creation_msg_serveur("a rejoint le serveur",args.pseudo," ");
    envoie_everyone_serveur(*msgcomplet);

    lecture_envoie(args); //le client va pouvoir commencer a communiquer
    pthread_exit(0);
}

//initialise la communication entre le client et le serveur
//Entrée : le socket d'écoute
//Sortie : renvoie rien, lance la fonction du pseudo avec le socket créé si la communication peut s'opérer
void init_connexion(int dS) {

    for(int i = 0; i<NB_MAX_PERSONNE;i++) { //initialise le tableau
        tabdSC[i].dSC = -1;
    }

    struct sockaddr_in aC ; //structure du socket
    socklen_t lg = sizeof(struct sockaddr_in) ;  

    while(true){ //continue a s'éxecuter 

        int dSC = accept(dS, (struct sockaddr*) &aC,&lg); //crée le socket client
        if (dSC == -1) { //gestion de l'erreur de accept
            printf("problème de connexion\n");
        }
        else {
            pthread_mutex_lock(&M2); //bloque l'accès au compteur du nombre de personne
            if (NB_PERSONNE_ACTUELLE < NB_MAX_PERSONNE){
                
                int a = 0;
                send(dSC, &a, sizeof(int), 0);

                printf("Client Connecté\n");            
                NB_PERSONNE_ACTUELLE++;

                pthread_mutex_unlock(&M2); //redonne l'accès au nombre de client connecté

                pthread_t thread;

                pthread_mutex_lock(&M1); //bloque l'accès au tableau
                int i = 0;
                while(tabdSC[i].dSC != -1){ //si on trouve un slot de libre (qui existe forcément par la vérification fait au préalable)
                    i = (i + 1)%NB_MAX_PERSONNE;
                }

                tabdSC[i].thread;
                pthread_mutex_unlock(&M1); //on redonne l'accès

                struct Args_Thread args;
                args.dSC = dSC;
                args.id = i;

                pthread_create(&thread, NULL, choixPseudo,(void*)&args); //on lance le thread du client
            }
            else {
                int a = 1;
                send(dSC, &a, sizeof(int), 0);
                shutdown(dSC,2); //fin du socket car il n'y a plus de place dans la communication
            }
        }
    }
}

void ArretForce(int n) {
    printf("Coupure du programme");
    envoie_everyone_serveur("fermeture du serveur\n");
    pthread_mutex_lock(&M1);
    for (int i = 0;i<NB_MAX_PERSONNE+1;i++) {
        shutdown(tabdSC[i].dSC,2);
    }
    pthread_mutex_unlock(&M1);
    exit(0);
}

//main de la fonciton
int main(int argc, char *argv[]) { 

    printf("Début programme\n");

    signal(SIGINT, ArretForce); //Ajout du signal de fin ctrl+c

    int dS = init_ouverture_connexion(atoi(argv[1])); //on crée le socket de communication 

    init_connexion(dS); //lancement de la connexion des clients
    shutdown(dS,2); //fin du socket de communication
    printf("fin programme");
}