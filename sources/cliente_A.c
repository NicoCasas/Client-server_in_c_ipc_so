/**
 * Cliente que usa fifo o named pipes
*/
/* #define _XOPEN_SOURCE 700 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "../include/variables_entorno.h"


#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/un.h>


#define CADENA_SIZE 64

void enviar_mensaje(char* cadena, int sfd);
void obtener_mensaje(char* cadena);
void configurar_sigint();

#define CLIENTE_A_PROMPT "Cliente_A: "
void leer_cadena_de_command_line(char *cadena);

#define UNIX_PATH   "/tmp/ffalkjdflkjasdnflkjasndflas"
int establecer_comunicacion_con_servidor    (void);

//////////////////////////////////////////////////////////////////////////////
/// Referido a al clean-up at exit

/**
 * SIGINT HANDLER
*/
void sigint_handler(int num){
    num++;  //Por el warning de no usar num
    exit(1);
}

/////////////////////////////////////MAIN//////////////////////////////////////////

int main(int argc, char* argv[]){
    char cadena[CADENA_SIZE];
    char config_path[CADENA_SIZE] = CONFIG_FILE_PATH_DEFAULT;
    int sfd;

    configurar_sigint();
    if(argc>1){
        strncpy(config_path,argv[1],CADENA_SIZE);
    }

    cargar_variables_de_entorno_de_archivo(config_path);    
    //Falta comprobar y cargar las que correspondan
    sfd = establecer_comunicacion_con_servidor();

    //while(1){
    for(int i=0; i<2; i++){
        memset(cadena,0,CADENA_SIZE);    
        obtener_mensaje(cadena);
        enviar_mensaje(cadena,sfd);
    }
    
    return 0;
}

void configurar_sigint(){
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = sigint_handler;

    sigaction(SIGINT, &sa, NULL);
}

int establecer_comunicacion_con_servidor(void){
    int sfd;
    struct sockaddr_un addr;

    sfd = socket(AF_UNIX,SOCK_STREAM,0);
    if(sfd == -1){
        perror("Error creando socket");
        exit(1);
    }

    memset(&addr,0,sizeof(struct sockaddr_un));
    
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path,UNIX_PATH, sizeof(addr.sun_path)-1);

    if(connect(sfd,&addr,sizeof(struct sockaddr_un))==-1){
        perror("Error conectando");
        exit(1);
    }

    return sfd;

}


void obtener_mensaje(char* cadena){
    leer_cadena_de_command_line(cadena);
}

void leer_cadena_de_command_line(char *cadena){
    printf(CLIENTE_A_PROMPT);
    if(fgets(cadena,CADENA_SIZE,stdin)==NULL){
        perror("Error leyendo cadena de command line");
        exit(1);
    }
}

void enviar_mensaje(char* cadena, int sfd){
    printf("Mando: %s\n",cadena);
}

