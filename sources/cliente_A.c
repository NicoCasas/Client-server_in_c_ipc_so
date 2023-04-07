/**
 * Cliente que usa AF_UNIX sockets
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
#include "checksum.h"
#include "variables_entorno.h"


#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/un.h>


#define CADENA_SIZE 64

void enviar_mensaje(char* cadena, int sfd);
void obtener_mensaje(char* cadena);
void configurar_sigint();
size_t leer_meminfo(char* mensaje);

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
    char mensaje[2000];
    memset(mensaje,0,2000);
    size_t data_len = leer_meminfo(mensaje);
    send_data_msg(sfd,mensaje,data_len);

    for(int i=0; i<2; i++){
        memset(cadena,0,CADENA_SIZE);    
        obtener_mensaje(cadena);
        enviar_mensaje(cadena,sfd);
    }
    
    return 0;
}

size_t leer_meminfo(char* mensaje){
    FILE* fp = fopen("/proc/meminfo","r");
    if(fp==NULL){
        perror("Error abirendo archivo");
        exit(1);
    }
    size_t bytes_read = fread(mensaje,sizeof(char),2000,fp);
    fclose(fp);
    return bytes_read;
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
    cadena[strlen(cadena)-1]='\0'; //Removemos el salto de linea
}

void leer_cadena_de_command_line(char *cadena){
    printf(CLIENTE_A_PROMPT);
    if(fgets(cadena,CADENA_SIZE,stdin)==NULL){
        perror("Error leyendo cadena de command line");
        exit(1);
    }
}

void enviar_mensaje(char* cadena, int sfd){
    size_t len;
    ssize_t bytes_send;
    char* mensaje = get_msg_to_transmit(1,0,(unsigned int)strlen(cadena),cadena,&len);

    bytes_send = send(sfd,mensaje,len,0);
    if(bytes_send < 0){
        perror("Error enviando mensaje");
        exit(1);
    }
    printf("Mando: %s\n",cadena);
    
    free(mensaje);
    return;
}

