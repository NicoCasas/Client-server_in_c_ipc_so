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
#include "cJSON.h"
#include "cJSON_custom.h"

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/un.h>


#define CADENA_SIZE 64

void    enviar_mensaje          (char* cadena, int sfd);
char*   obtener_mensaje         (void);
void    configurar_sigint       ();
size_t  leer_meminfo            (char* mensaje);
void    free_matrix             (char** matrix);

#define CLIENTE_A_PROMPT "Cliente_A: "
void leer_cadena_de_command_line(char *cadena);

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
    char config_path[CADENA_SIZE] = CONFIG_FILE_PATH_DEFAULT;
    int sfd;
    char* a_enviar;
    char* respuesta = NULL;
    size_t len_respuesta=0;
    char** responses = NULL;

    configurar_sigint();
    if(argc>1){
        strncpy(config_path,argv[1],CADENA_SIZE);
    }

    cargar_variables_de_entorno_de_archivo(config_path);   
    comprobar_variables_entorno_A(); 
    //Falta comprobar y cargar las que correspondan
    sfd = establecer_comunicacion_con_servidor();

    //while(1){
    for(int i=0; i<2; i++){
        a_enviar = obtener_mensaje();
        enviar_mensaje(a_enviar,sfd);

        respuesta = receive_data_msg(sfd,&len_respuesta);
        if(respuesta == NULL){
            switch(len_respuesta){
                case 0: printf("El sv se desconectó\n");break;
                case 1: printf("Checksum invalido");break;
                default: break;
            }
            free(a_enviar);
            continue;
        }

        responses = cJSON_get_responses(respuesta,NULL);

        printf("%s\n",responses[0]);

        free(respuesta);
        free(a_enviar);
        free_matrix(responses);
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
    strncpy(addr.sun_path,getenv(UNIX_PATH_ENV_NAME), sizeof(addr.sun_path)-1);

    if(connect(sfd,&addr,sizeof(struct sockaddr_un))==-1){
        perror("Error conectando");
        exit(1);
    }

    return sfd;

}


char* obtener_mensaje(void){
    char* mensaje = NULL;
    char cadena[CADENA_SIZE];
    
    cJSON* requests = NULL;
    cJSON* monitor = cJSON_get_header_request_by_client(&requests,"Cliente A");

    // For testing
    cJSON_add_pid(monitor);

    // Requests
    memset(cadena,0,CADENA_SIZE);
    leer_cadena_de_command_line(cadena);
    cJSON_add_string_to_array(requests,"request_1",cadena);

    // n_request
    cJSON_replace_number_value(monitor,"n_requests",cJSON_GetArraySize(requests));

    // Impresion
    mensaje = cJSON_Print(monitor);
    if(mensaje==NULL){
        perror("Error imprimiendo json");
        exit(1);
    }

    cJSON_Delete(monitor);

    return mensaje;
}

void leer_cadena_de_command_line(char *cadena){
    printf(CLIENTE_A_PROMPT);
    if(fgets(cadena,CADENA_SIZE,stdin)==NULL){
        perror("Error leyendo cadena de command line");
        exit(1);
    }
    cadena[strlen(cadena)-1]='\0'; //Removemos el salto de linea

}

void enviar_mensaje(char* a_enviar, int sfd){
    ssize_t bytes_send;

    bytes_send = send_data_msg(sfd,a_enviar,strlen(a_enviar),MSG_DONTWAIT);
    if(bytes_send < 0){
        perror("Error enviando mensaje");
        exit(1);
    }
    //printf("Mando: %s\n",a_enviar);
    
    return;
}


/**
 * Frees a two dimensional array
*/
void free_matrix(char** matrix){
    int i = 0;
    while(matrix[i]!=NULL){
        free (matrix[i]);
        i++;
    }
    free(matrix);
}