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
#include <time.h>
#include <errno.h>
#include "variables_entorno.h"
#include "cJSON.h"
#include "cJSON_custom.h"

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define FECHA_SIZE 20
#define CADENA_SIZE 64
#define COMPRESSED_FILE_EXTENSION    ".txt.gz"
#define LEN_COMPRESSED_FILE_EXTENSION       7

void crear_path_archivos_comprimidos(void);
void enviar_mensaje(char* cadena, int sfd);
char* obtener_mensaje(void);
void configurar_sigint();
void procesar_respuesta(char* respuesta, size_t len_respuesta);
char* obtener_fecha();

#define CLIENTE_A_PROMPT "Cliente_B: "
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
    char* a_enviar = NULL;
    char* respuesta= NULL;
    size_t len_respuesta;

    configurar_sigint();
    if(argc>1){
        strncpy(config_path,argv[1],CADENA_SIZE);
    }

    cargar_variables_de_entorno_de_archivo(config_path);
    comprobar_variables_entorno_B();
    crear_path_archivos_comprimidos();
    //Falta comprobar y cargar las que correspondan
    sfd = establecer_comunicacion_con_servidor();

    while(1){
    //for(int i=0; i<2; i++){
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

        if(strncmp(respuesta,"Error: Comando invalido",strlen(respuesta))){ //Entra si no es error
            procesar_respuesta(respuesta,len_respuesta);
            printf("Archivo comprimido correctamente descargado\n");    
        }
        else{
            printf("Error: Comando invalido\n");
        }
        free(respuesta);
        free(a_enviar);
    }
    
    return 0;
}

void crear_path_archivos_comprimidos(void){
    int flag = mkdir(getenv(COMPRESS_PATH_ENV_NAME),0644);
    if(flag==-1 && errno!=EEXIST){
        perror("Error creando archivo");
        exit(1);
    }
}

void procesar_respuesta(char* respuesta, size_t len_respuesta){
    char path[CADENA_SIZE];
    //FILE* fp;
    ssize_t bytes_write=0;
    int fd;

    memset(path,0,CADENA_SIZE);
    char* fecha = obtener_fecha();
    sprintf(path,"%s%s_XXXXXX%s",getenv(COMPRESS_PATH_ENV_NAME),fecha,COMPRESSED_FILE_EXTENSION);
    
    fd=mkstemps(path,LEN_COMPRESSED_FILE_EXTENSION);
    if(fd==-1){
        perror("Error creando path");
        exit(1);
    }

    bytes_write = write(fd,respuesta,len_respuesta);
    if(bytes_write==0){
        perror("Error escribiendo archivo comprimido");
        exit(1);
    }
    
    if(close(fd)!=0){
        perror("Error cerrando archivo");
        exit(1);
    }

    free(fecha);
    return;
    
}

/**
 * Retorna un puntero dinámicamente alocado con la fecha en formato año-mes-dia hora:min:seg
 * Extraido y ligeramente modificado de 
 * https://stackoverflow.com/questions/1442116/how-to-get-the-date-and-time-values-in-a-c-program
 * 
*/
char* obtener_fecha(){
    char* fecha = calloc(sizeof(char),FECHA_SIZE);
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(fecha,"%d-%02d-%02d_%02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    
    return fecha;
}

void configurar_sigint(){
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = sigint_handler;

    sigaction(SIGINT, &sa, NULL);
}

int establecer_comunicacion_con_servidor(void){
    int sfd;
    struct sockaddr_in addr;
    int port;

    sfd = socket(AF_INET,SOCK_STREAM,0);
    if(sfd == -1){
        perror("Error creando socket");
        exit(1);
    }

    memset(&addr,0,sizeof(struct sockaddr_in));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr(getenv(IPV4_IP_ENV_NAME));
    sscanf(getenv(IPV4_PORT_ENV_NAME),"%d",&port);
    addr.sin_port        = htons((uint16_t)port);

    if(connect(sfd,(struct sockaddr*)&addr,sizeof(struct sockaddr_in))==-1){
        perror("Error conectando");
        exit(1);
    }

    return sfd;
}


char* obtener_mensaje(void){
    char* mensaje = NULL;
    char cadena[CADENA_SIZE];
    
    cJSON* requests = NULL;
    cJSON* monitor = cJSON_get_header_request_by_client(&requests,"Cliente B");

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
    while(cadena[0]=='\0'){
        printf(CLIENTE_A_PROMPT);
        if(fgets(cadena,CADENA_SIZE,stdin)==NULL){
            perror("Error leyendo cadena de command line");
            exit(1);
        }
        cadena[strlen(cadena)-1]='\0'; //Removemos el salto de linea
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
    //printf("Mando: %s\n",cadena);
    
    free(mensaje);
    return;
}

