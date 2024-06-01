#include <pthread.h>
#include <stdbool.h>
#include <pthread.h>

#define NB_MAX_PERSONNE_SALON 100 //limite max du nombre de personne dans un salon
#define NB_MAX_PERSONNE 100 //limite max de personne sur le serveur
#define MAX_FILE 200 //limite max de personne sur le serveur

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
    int id;
    int dSC; //socket du client
    char* pseudo; //son pseudo
    pthread_t thread; //son thread
    int id_salon;
    bool isAdmin;
};

struct salon{
    int id;
    char* nom;
    int client[NB_MAX_PERSONNE_SALON];
    int NB_MAX;
};

struct client tabdSC[NB_MAX_PERSONNE]; //tableau des sockets des clients
