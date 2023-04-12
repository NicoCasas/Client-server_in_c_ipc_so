#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "cJSON.h"
#include "cJSON_custom.h"

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