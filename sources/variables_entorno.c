#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <sys/types.h>
#include "../include/variables_entorno.h"

#define CADENA_SIZE     64
#define REGEX_CONFIG    "^([A-Z_][A-Z0-9_]*=[^ \n][^\n]*\n?)*$"

//Definicion de funciones que no quiero que se vean desde afuera
static char*   leer_archivo_dinamicamente                              (const char* path);
static int     comprobar_sintaxis_archivo_configuracion                (char* buffer);
static void    cargar_variables_de_entorno                             (char* buffer);
static void    imprimir_mensaje_error_sintaxis_archivo_configuracion   (void);
static void    verificar_archivo_y_cargar_variables_de_entorno         (const char* path);

void comprobar_variables_entorno(void){
    printf("Variables configuradas por default: \n");
    comprobar_variables_entorno_A();
    comprobar_variables_entorno_B();
    comprobar_variables_entorno_C();
    comprobar_variables_entorno_log();
    printf("\n");
}

void comprobar_variables_entorno_A(void){

    if(getenv(UNIX_PATH_ENV_NAME)==NULL){
        setenv(UNIX_PATH_ENV_NAME,UNIX_PATH_DEFAULT,0);
        printf("%s=%s\n",UNIX_PATH_ENV_NAME,UNIX_PATH_DEFAULT);
    }
    return;
}

void comprobar_variables_entorno_B(void){
    if(getenv(IPV4_IP_ENV_NAME)==NULL){
        setenv(IPV4_IP_ENV_NAME,IPV4_IP_DEFAULT,0);
        printf("%s=%s\n",IPV4_IP_ENV_NAME,IPV4_IP_DEFAULT);
    }
    if(getenv(IPV4_PORT_ENV_NAME)==NULL){
        setenv(IPV4_PORT_ENV_NAME,IPV4_PORT_DEFAULT,0);
        printf("%s=%s\n",IPV4_PORT_ENV_NAME,IPV4_PORT_DEFAULT);
    }
    if(getenv(COMPRESS_PATH_ENV_NAME)==NULL){
        setenv(COMPRESS_PATH_ENV_NAME,COMPRESS_PATH_DEFAULT,0);
        printf("%s=%s\n",COMPRESS_PATH_ENV_NAME,COMPRESS_PATH_DEFAULT);
    }
    return;
}

void comprobar_variables_entorno_C(void){
    if(getenv(IPV6_IP_ENV_NAME)==NULL){
        setenv(IPV6_IP_ENV_NAME,IPV6_IP_DEFAULT,0);
        printf("%s=%s\n",IPV6_IP_ENV_NAME,IPV6_IP_DEFAULT);
    }
    if(getenv(IPV6_PORT_ENV_NAME)==NULL){
        setenv(IPV6_PORT_ENV_NAME,IPV6_PORT_DEFAULT,0);
        printf("%s=%s\n",IPV6_PORT_ENV_NAME,IPV6_PORT_DEFAULT);
    }
    return;
}

void comprobar_variables_entorno_log(void){
    if(getenv(LOG_PATH_ENV_NAME)==NULL){
        setenv(LOG_PATH_ENV_NAME,LOG_PATH_DEFAULT,0);
        printf("%s=%s\n",LOG_PATH_ENV_NAME,LOG_PATH_DEFAULT);
    }
    return;
}

void cargar_variables_de_entorno_de_archivo(const char* path){
    verificar_archivo_y_cargar_variables_de_entorno(path);
}

static void verificar_archivo_y_cargar_variables_de_entorno(const char* path){
    char* buffer = NULL;    

    printf("\nLeyendo variables del archivo de configuracion: %s\n\n",path);
    buffer = leer_archivo_dinamicamente(path);    

    if(comprobar_sintaxis_archivo_configuracion(buffer)!=0){
        imprimir_mensaje_error_sintaxis_archivo_configuracion();
        free(buffer);
        return;
    }
    cargar_variables_de_entorno(buffer);
    
    free(buffer);
}


static void cargar_variables_de_entorno(char* buffer){
    char* aux;
    char* separator_pos;

    char env_name[CADENA_SIZE];
    char env_value[CADENA_SIZE];
    
    printf("Variables de entorno configuradas por archivo suministrado:\n\n");
    aux = strtok(buffer,"\n");
    while(aux!=NULL){
        //Do stuff
        separator_pos = strchr(aux,'=');

        memset(env_name,0,CADENA_SIZE);
        memset(env_value,0,CADENA_SIZE);

        strncpy(env_name,aux,(size_t) (separator_pos - aux));
        strncpy(env_value,separator_pos+1,(size_t)(aux+strlen(aux)-(separator_pos+1)));

        setenv(env_name,env_value,1);
        printf("%s=%s\n",env_name,getenv(env_name));
        //Move on
        aux = strtok(NULL,"\n");
    }

    printf("\n");
}

static void imprimir_mensaje_error_sintaxis_archivo_configuracion(void){
    printf("Error en la sintaxis del archivo de configuracion\n");
    printf("Debe ser lineas de la siguiente forma:\tKEY=Value\n");
    printf("KEY debe estar en mayusculas. Puede contener el caracter '_'. Value es libre\n\n");
}

static char* leer_archivo_dinamicamente(const char* path){
    FILE* fp;
    long size;
    char* buffer = NULL;
    size_t bytes_read;

    fp = fopen(path,"r");
    if(fp==NULL){
        perror("Error abriendo archivo dinamicamente");
        exit(1);
    }

    if(fseek(fp,0,SEEK_END)==-1){
        perror("Error fseeking");
        exit(1);
    }
    size = ftell(fp);    
    if(fseek(fp,0,SEEK_SET)==-1 || size==-1){
        perror("Error fseeking");
        exit(1);
    }

    buffer = malloc((size_t)size+1);
    bytes_read = fread(buffer,sizeof(char),(size_t)size,fp);
    if(bytes_read==0){
        printf("Archivo de configuracion vacio\n");
        //exit(1);
    }
    buffer[size]='\0';

    fclose(fp);

    return buffer;
}

static int comprobar_sintaxis_archivo_configuracion(char* buffer){
    regex_t regex;
    int resultado;

    if(regcomp(&regex,REGEX_CONFIG,REG_EXTENDED)!=0){
        perror("Error compilando regex");
        exit(1);
    }
    resultado = regexec(&regex,buffer,0,NULL,0);

    regfree(&regex);
    return resultado;
}