int main(int argc, char *argv[]) {
  
    printf("Début programme\n");

    int dS = socket(PF_INET, SOCK_STREAM, 0);
    printf("Socket Créé\n");


    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY ;
    ad.sin_port = htons(atoi(argv[1])) ;
    bind(dS, (struct sockaddr*)&ad, sizeof(ad)) ;
    printf("Socket Nommé\n");

    listen(dS, 7) ;
    printf("Mode écoute\n");

    struct sockaddr_in aC ;
    socklen_t lg1 = sizeof(struct sockaddr_in) ;
    int dSC1 = accept(dS, (struct sockaddr*) &aC,&lg1) ;
    printf("Client 1 Connecté\n");

        struct sockaddr_in aC ;
    socklen_t lg2 = sizeof(struct sockaddr_in) ;
    int dSC2 = accept(dS, (struct sockaddr*) &aC,&lg2) ;
    printf("Client 2 Connecté\n");

    char msg [20] ;
    recv(dSC, msg, sizeof(msg), 0) ;
    printf("Message reçu : %s\n", msg) ;
    
    int r = 10 ;
    
    send(dSC, &r, sizeof(int), 0) ;
    printf("Message Envoyé\n");
    shutdown(dSC, 2) ; 
    shutdown(dS, 2) ;
    printf("Fin du programme");
}