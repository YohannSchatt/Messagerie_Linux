#include <pthread.h>

#define NB_MAX_PERSONNE_SALON 5 //limite max du nombre de personne dans un salon

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
    int id_salon;
};

struct salon{
    int id;
    char* prop;
    char* nom;
    struct client client[NB_MAX_PERSONNE_SALON];
};
