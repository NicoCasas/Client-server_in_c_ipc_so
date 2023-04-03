#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <sys/types.h>
#include "../include/variables_entorno.h"

#define CADENA_SIZE     64
#define REGEX_CONFIG    "^([A-Z_]+=[^ \n][^\n]*\n?)*$"

//Definicion de funciones que no quiero que se vean desde afuera
char*   leer_archivo_dinamicamente                              (const char* path);
int     comprobar_sintaxis_archivo_configuracion                (char* buffer);
void    cargar_variables_de_entorno                             (char* buffer);
void    imprimir_mensaje_error_sintaxis_archivo_configuracion   ();
void    verificar_archivo_y_cargar_variables_de_entorno         (const char* path);

void comprobar_variables_entorno(){
    printf("Variables configuradas por default: \n");
    comprobar_variables_entorno_A();
    comprobar_variables_entorno_B();
    comprobar_variables_entorno_C();
    comprobar_variables_entorno_log();
    printf("\n");
}

void comprobar_variables_entorno_A(){
    /* if(getenv(FIFO_PATH_ENV_NAME)==NULL){
        setenv(FIFO_PATH_ENV_NAME,FIFO_PATH_DEFAULT,0);
        printf("%s=%s\n",FIFO_PATH_ENV_NAME,FIFO_PATH_DEFAULT);
    } */
    return;
}

void comprobar_variables_entorno_B(){
    /* if(getenv(MSGQ_PATH_ENV_NAME)==NULL){
        setenv(MSGQ_PATH_ENV_NAME,MSGQ_PATH_DEFAULT,0);
        printf("%s=%s\n",MSGQ_PATH_ENV_NAME,MSGQ_PATH_DEFAULT);
    } */
    return;
}

void comprobar_variables_entorno_C(){
    /* if(getenv(SHM_PATH_ENV_NAME)==NULL){
        setenv(SHM_PATH_ENV_NAME,SHM_PATH_DEFAULT,0);
        printf("%s=%s\n",SHM_PATH_ENV_NAME,SHM_PATH_DEFAULT);
    }
    if(getenv(SEM_LECTOR_F_ENV_NAME)==NULL){
        setenv(SEM_LECTOR_F_ENV_NAME,SEM_LECTOR_FNAME_DEFAULT,0);
        printf("%s=%s\n",SEM_LECTOR_F_ENV_NAME,SEM_LECTOR_FNAME_DEFAULT);
    }
    if(getenv(SEM_ESCRITOR_F_ENV_NAME)==NULL){
        setenv(SEM_ESCRITOR_F_ENV_NAME,SEM_ESCRITOR_FNAME_DEFAULT,0);
        printf("%s=%s\n",SEM_ESCRITOR_F_ENV_NAME,SEM_ESCRITOR_FNAME_DEFAULT);
    } */
    return;
}

void comprobar_variables_entorno_log(){
    if(getenv(LOG_PATH_ENV_NAME)==NULL){
        setenv(LOG_PATH_ENV_NAME,LOG_PATH_DEFAULT,0);
        printf("%s=%s\n",LOG_PATH_ENV_NAME,LOG_PATH_DEFAULT);
    }
    return;
}

void cargar_variables_de_entorno_de_archivo(const char* path){
    verificar_archivo_y_cargar_variables_de_entorno(path);
}

void verificar_archivo_y_cargar_variables_de_entorno(const char* path){
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


void cargar_variables_de_entorno(char* buffer){
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

void imprimir_mensaje_error_sintaxis_archivo_configuracion(){
    printf("Error en la sintaxis del archivo de configuracion\n");
    printf("Debe ser lineas de la siguiente forma:\tKEY=Value\n");
    printf("KEY debe estar en mayusculas. Puede contener el caracter '_'. Value es libre\n\n");
}

char* leer_archivo_dinamicamente(const char* path){
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

int comprobar_sintaxis_archivo_configuracion(char* buffer){
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