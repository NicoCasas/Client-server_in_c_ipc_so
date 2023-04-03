/**
 * Cliente que usa fifo o named pipes
*/
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "../include/variables_entorno.h"

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>


#define CADENA_SIZE 64

void enviar_mensaje(char* cadena);
void obtener_mensaje(char* cadena);
void configurar_sigint();

#define CLIENTE_A_PROMPT "Cliente_A: "
void leer_cadena_de_command_line(char *cadena);

//////////////////////////////////////////////////////////////////////////////
/// Referido a al clean-up at exit

/**
 * SIGINT HANDLER
*/
void sigint_handler(int num){
    num++;  //Por el warning de no usar num
    exit(1);
}

int main(int argc, char* argv[]){
    char cadena[CADENA_SIZE];
    char config_path[CADENA_SIZE] = CONFIG_FILE_PATH_DEFAULT;
    
    configurar_sigint();
    if(argc>1){
        strncpy(config_path,argv[1],CADENA_SIZE);
    }

    cargar_variables_de_entorno_de_archivo(config_path);    
    //Falta comprobar y cargar las que correspondan
    //establecer_comunicacion_con_servidor();

    //while(1){
    for(int i=0; i<2; i++){
        memset(cadena,0,CADENA_SIZE);    
        obtener_mensaje(cadena);
        enviar_mensaje(cadena);

    }
    
    return 0;
}

void configurar_sigint(){
 struct sigaction sa;
    memset(&sa,0,sizeof(sa));           //Para no tener errores de valgrind
    sa.sa_handler = sigint_handler;

    sigaction(SIGINT, &sa, NULL);
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

void enviar_mensaje(char* cadena){
    printf("Mando: %s\n",cadena);
}

