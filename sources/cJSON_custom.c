#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "cJSON.h"
#include "cJSON_custom.h"

static void* realloc_safe(void* ptr, size_t size);
static void* calloc_safe(size_t nmemb, size_t size);
/////////////////////////////////////////////////////////////////////////////
/// cJSON extra

void cJSON_add_number_to_array(cJSON* array, const char* name, double number){
    cJSON* object = NULL;
    cJSON* f1 = NULL;
    int f2 = 0;

    object = cJSON_CreateObject();
    if(object == NULL){
        perror("Error creando objeto en cJSON");
        exit(1);
    }

    f1 = cJSON_AddNumberToObject(object,name,number);
    f2 = cJSON_AddItemToArray(array,object);

    if(f1 == NULL || f2 == 0){
        perror("Error creando response en cJSON");
        exit(1);
    }
}

void cJSON_add_string_to_array(cJSON* array, const char* name, const char* string){
    cJSON* object = NULL;
    cJSON* f1 = NULL;
    int f2 = 0;

    object = cJSON_CreateObject();
    if(object == NULL){
        perror("Error creando objeto en cJSON");
        exit(1);
    }

    f1 = cJSON_AddStringToObject(object,name,string);
    f2 = cJSON_AddItemToArray(array,object);

    if(f1 == NULL || f2 == 0){
        perror("Error creando response en cJSON");
        exit(1);
    }
}

void cJSON_replace_number_value(cJSON* monitor, const char* key, double new_value){
    cJSON* object = cJSON_CreateNumber(new_value);
    if(object==NULL){
        perror("Error creating object");
        exit(1);
    }

    if(cJSON_ReplaceItemInObject(monitor,key,object)==0){
        perror("Error replacing number");
        exit(1);
    }
    return;
}

/**
 * Forma:
 * {
 *      "origen":   "servidor",
 *      "n_responses":   0
 *      "responses":  [
 *              
 *      ] 
 * }
*/
cJSON* cJSON_get_header_response_by_server(cJSON** responses_p){

    cJSON *f1=NULL, *f2=NULL;
    cJSON* monitor = cJSON_CreateObject();
    
    if(monitor == NULL){
        perror("Error creando monitor");
        exit(1);
    }

    f1 = cJSON_AddStringToObject(monitor,"origen","servidor");
    f2 = cJSON_AddNumberToObject(monitor,"n_responses",0);
    *responses_p = cJSON_AddArrayToObject(monitor,"responses");
    
    if(f1 == NULL || f2 == NULL ||*responses_p == NULL){
        perror("Error creando elementos de cJSON");
        exit(1);
    }

    return monitor;
}

cJSON* cJSON_get_header_request_by_client(cJSON** requests_p, const char* client_name){
    
    cJSON *f1=NULL, *f2=NULL;
    cJSON* monitor = cJSON_CreateObject();
    
    if(monitor == NULL){
        perror("Error creando monitor");
        exit(1);
    }

    f1 = cJSON_AddStringToObject(monitor,"origen",client_name);
    f2 = cJSON_AddNumberToObject(monitor,"n_requests",0);
    *requests_p = cJSON_AddArrayToObject(monitor,"requests");
    
    if(f1 == NULL || f2 == NULL ||*requests_p == NULL){
        perror("Error creando elementos de cJSON");
        exit(1);
    }

    return monitor;
}

void cJSON_add_pid(cJSON* monitor){
    cJSON *f1 = NULL;

    f1 = cJSON_AddNumberToObject(monitor,"pid",getpid());
    if(f1 == NULL){
        perror("Error creando elementos de cJSON");
        exit(1);
    }

}

/**
 * Returns a pointer to a dynamically allocated 2-dimensional array that contains the requests splited.
 * In other words, each element of the array is an individual request.
 * It must be freed.
 * If n_requests_p is not NULL, stores the number of elements of the array.
*/
char** cJSON_get_requests(char* mensaje, unsigned int* n_requests_p){
    char** requests_arr=NULL;
    char* request = NULL;
    cJSON* monitor = NULL;
    cJSON* request_obj  = NULL;
    unsigned int contador = 0;
    char key[KEY_SIZE];

    monitor = cJSON_Parse(mensaje);
    if(monitor == NULL){
        perror("Error parseando json");
        exit(1);
    }
    
    cJSON* requests = cJSON_GetObjectItem(monitor,"requests");
    if(requests == NULL){
        perror("Error obteniendo requests");
        exit(1);
    }

    cJSON_ArrayForEach(request_obj,requests){
        memset(key,0,KEY_SIZE);
        sprintf(key,"request_%d",contador+1);       //Por alguna razon hice que las requests empezaran en 1
        cJSON* aux = cJSON_GetObjectItem(request_obj,key); 
        if(aux==NULL){
            perror("Error obteniendo item del array");
            exit(1);
        }

        request = cJSON_GetStringValue(aux);
        requests_arr = realloc_safe(requests_arr,sizeof(char*)*(contador+1));
        requests_arr[contador] = calloc_safe(strlen(request)+1,sizeof(char));
        strncpy(requests_arr[contador],request,strlen(request));

        contador++;
    }

    requests_arr = realloc_safe(requests_arr,sizeof(char*)*(contador+1));
    requests_arr[contador] = NULL;

    cJSON_Delete(monitor);

    if(n_requests_p!=NULL){
        *n_requests_p = contador;
    }

    return requests_arr;
}

static void* realloc_safe(void* ptr, size_t size){
    void* aux = realloc(ptr,size);
    if(aux==NULL){
        perror("Error realocando");
        exit(1);
    }
    return aux;
}

static void* calloc_safe(size_t nmemb, size_t size){
    void* aux = calloc(nmemb,size);
    if(aux == NULL){
        perror("Error alocando");
        exit(1);
    }
    return aux;
}