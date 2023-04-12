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
#include <checksum.h>
#include "variables_entorno.h"
#include "cJSON.h"
#include "cJSON_custom.h"


#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define CADENA_SIZE     64
#define PID_BUFF_SIZE   16

void enviar_mensaje(char* cadena, int sfd);
char* obtener_mensaje(void);
void obtener_string_pid(char* pid_buf, size_t len);

void configurar_sigint();

#define CLIENTE_A_PROMPT "Cliente_C: "
void leer_cadena_de_command_line(char *cadena);

#define IPV6_IP                             "::1" //0:0:0:0:0:0:1
#define IPV6_PORT                            5060

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
    char* mensaje = NULL;
    char* respuesta = NULL;
    size_t len_respuesta=0;

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
        mensaje = obtener_mensaje();
        enviar_mensaje(mensaje,sfd);        

        respuesta = receive_data_msg(sfd,&len_respuesta);
        if(respuesta == NULL){
            switch(len_respuesta){
                case 0: printf("El sv se desconectó\n");break;
                case 1: printf("Checksum invalido");break;
                default: break;
            }
            continue;
        }

        printf("%s\n",respuesta);
        
        free(mensaje);
        free(respuesta);

        if(sleep(1)!=0){
            exit(1);
        }
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
    struct sockaddr_in6 addr;

    sfd = socket(AF_INET6,SOCK_STREAM,0);
    if(sfd == -1){
        perror("Error creando socket");
        exit(1);
    }

    memset(&addr,0,sizeof(struct sockaddr_in6));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(IPV6_PORT);

    if(inet_pton(AF_INET6,IPV6_IP,&addr.sin6_addr)==-1){
        perror("Error convirtiendo direccion ipv6");
        exit(1);
    }
    
    if(connect(sfd,(struct sockaddr*)&addr,sizeof(struct sockaddr_in6))==-1){
        perror("Error conectando");
        exit(1);
    }

    return sfd;
}

/**
 * Forma:
 * {
 *      "origen":   "Cliente C",
 *      "n_request": 2,
 *      "pid":123,
 *      "requests":  [
 *              { "request_1" : mem_free        },
 *              { "request_2" : norm_load_avg   }
 *      ] 
 * }
*/
char* obtener_mensaje(void){
    char* mensaje = NULL;
    
    cJSON* requests = NULL;
    cJSON* monitor = cJSON_get_header_request_by_client(&requests,"Cliente C");

    // For testing
    cJSON_add_pid(monitor);

    // Requests
    cJSON_add_string_to_array(requests,"request_1","mem_free");
    cJSON_add_string_to_array(requests,"request_2","norm_load_avg");

    cJSON_replace_number_value(monitor,"n_requests",cJSON_GetArraySize(requests));

    mensaje = cJSON_Print(monitor);
    if(mensaje==NULL){
        perror("Error imprimiendo json");
        exit(1);
    }

    cJSON_Delete(monitor);

    return mensaje;

}

void obtener_string_pid(char* pid_buf, size_t len){
    memset(pid_buf,0,len); 
    sprintf(pid_buf,"%d",getpid());    
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
    char* a_enviar = get_msg_to_transmit(1,0,(unsigned int)strlen(cadena),cadena,&len);

    bytes_send = send(sfd,a_enviar,len,0);
    if(bytes_send < 0){
        perror("Error enviando mensaje");
        exit(1);
    }
    printf("Mando: %s\n",cadena);
    printf("Largo: %ld\n",strlen(cadena));
    free(a_enviar);
    
    return;
}

