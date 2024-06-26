// Esto es solamente para que la ide no moleste con no encontrar la estructura de sigaction
#define _GNU_SOURCE             /* See feature_test_macros(7) */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/ipc.h>
#include "variables_entorno.h"
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/epoll.h>
#include "checksum.h"
#include "cJSON.h"
#include "cJSON_custom.h"
#include "compress_and_decompress_file.h"

#define CADENA_SIZE                                 N_BYTES_TO_RECEIVE
#define FECHA_SIZE                                  20

void    configurar_at_exit_y_sigint                 (void);
void    crear_archivo_si_no_existe                  (const char* path);

//////////////////////////////////////////////////////////////////////////////
/// Referido a comunicacion en general

typedef union{
    struct sockaddr_un*      addr_un;
    struct sockaddr_in*      addr_in;
    struct sockaddr_in6*     addr_in6;
}addr_union;

struct listener{
    int                           sfd;
    addr_union                  *addr;
    SLIST_ENTRY(listener)   listeners;
};
SLIST_HEAD(slist_head,listener);

struct cliente{
    int                            sfd;
    SLIST_ENTRY(cliente)      clientes;
};
SLIST_HEAD(clientes_head,cliente);

struct clientes_head clientes_A_head;
struct clientes_head clientes_B_head;
struct clientes_head clientes_C_head;

void agregar_cliente_a_lista    (struct clientes_head* clientes_head_p, int sfd);
void eliminar_lista_clientes    (struct clientes_head* clientes_head_p);

#define VALGRIND_ENV_NAME          "VALGRIND"
#define MAX_EV                         65536
#define S_LIMIT_FOPEN                  65600
#define H_LIMIT_FOPEN                  65600

int         crear_y_bindear_unix_socket             (const char* socket_path);
int         crear_y_bindear_inet_socket             (const char* ip, uint16_t port);//,uint16_t port);
int         crear_y_bindear_inet6_socket            (const char* ip, uint16_t port);
void        agregar_socket_a_epoll                  (int efd, int sfd);
void        aumentar_limite_de_archivos_abiertos    (void);
void        aceptar_cliente_y_agregar_a_estructuras (int list_sfd, struct sockaddr* addr_p, unsigned int* addr_len_p, 
                                                     int efd, struct clientes_head* ch_p);
char*       recibir_mensaje                         (int sfd, int efd);
void        enviar_mensaje_a_lista_clientes         (struct clientes_head* ch_p);
//////////////////////////////////////////////////////////////////////////////
/// Referido a cliente A

#define MAX_SIZE_CL_A (1<<28)  // 256 MB

int                 establecer_comunicacion_clientes_tipo_A     (const char* path_fifo);
void                procesar_mensajes_tipo_A                    (char* mensaje, int fd);
void                responder_mensaje_tipo_A                    (char* response, int sfd);
char*               armar_respuesta_tipo_A                      (char* response);
void                enviar_respuesta_tipo_A                     (char* respuesta, int sfd);
void                loguear_cliente_tipo_A                      (char* cadena);
void                imprimir_cantidad_de_mensajes_recibidos     (void);

//////////////////////////////////////////////////////////////////////////////
/// Referido a cliente B
#define TEMPORARY_COMPRESS_PATH   "compress.z"        //Path auxiliar para comprimir. Luego se elimina
#define MAX_SIZE_CL_B (1<<30)  // 1 GB

int     establecer_comunicacion_clientes_tipo_B     (const char* ip, uint16_t port);
void    procesar_mensajes_tipo_B                    (char* mensaje, int sfd);
void    comprimir_resultado                         (char* a_comprimir);
void    responder_mensaje_tipo_B                    (int sfd, char* resultado);
void    enviar_comprimido                           (int sfd);
void    loguear_cliente_tipo_B                      (char* cadena);

//////////////////////////////////////////////////////////////////////////////
/// Referido a cliente C

int     establecer_comunicacion_clientes_tipo_C     (const char* ip, uint16_t port);
void    procesar_mensajes_tipo_C                    (char* mensaje, int sfd);
void    loguear_cliente_tipo_C                      (char* cadena);
void    responder_mensajes_tipo_C                   (int sfd);
float   obtener_carga_normalizada                   (void);
int     obtener_cpu_cores                           (void);
float   obtener_carga                               (void);
int     obtener_memoria_libre                       (void);
void    leer_archivo                                (char* path, char* buffer);
char*   armar_respuesta_tipo_C                      (float carga_normalizada, int memoria_libre);
void    enviar_respuesta_tipo_C                     (char* mensaje, int sfd);

//////////////////////////////////////////////////////////////////////////////
/// Referido a logs

void    loguear                                     (char* cadena);
void    loguear_mensaje_cliente                     (char tipo,char* mensaje);
char*   obtener_fecha                               (void);

//////////////////////////////////////////////////////////////////////////////
/// Referido a obtener la salida de journalctl

char*   get_output_journalctl_command       (char* journalctl_command,size_t max_size);
void    esperar_hijo                        (pid_t pid);
void    set_pipe_as_stdout                  (int pipe_fds[2]);
char**  split_words_safely                  (char* buffer, unsigned int* n_items_p, char* pattern);
char**  split_args                          (char* command, unsigned int* n_args_p);
void*   calloc_safe                         (size_t __nmemb, size_t __size);
void*   realloc_safe                        (void* ptr, size_t size);
void    free_matrix                         (char** matrix);




/**
 * VARIABLES GLOBALES
*/

unsigned int n_mensajes_tipo_A;
unsigned int n_mensajes_tipo_B;
unsigned int n_mensajes_tipo_C;

char* g_err_msg;
size_t g_len_err_msg;

/**
 * At exit
*/
void limpiar_comunicaciones(void){

    enviar_mensaje_a_lista_clientes(&clientes_A_head);
    enviar_mensaje_a_lista_clientes(&clientes_B_head);
    enviar_mensaje_a_lista_clientes(&clientes_C_head);
    
    eliminar_lista_clientes(&clientes_A_head);
    eliminar_lista_clientes(&clientes_B_head);
    eliminar_lista_clientes(&clientes_C_head);

    free(g_err_msg);
    unlink(getenv(UNIX_PATH_ENV_NAME)); 
}

/**
 * SIGINT HANDLER
*/
void sigint_handler(int num){
    num++;  //Por el warning de no usar num
    exit(1);
}



//////////////////////////////////////MAIN///////////////////////////////////////

int main(int argc, char* argv[]){
    struct sockaddr_un cliente_A_addr;
    int listener_cliente_A_sfd;
    
    struct sockaddr_in cliente_B_addr;
    int listener_cliente_B_sfd;

    struct sockaddr_in6 cliente_C_addr;
    int listener_cliente_C_sfd;

    //Referido a carga de variables de entorno
    char config_path[CADENA_SIZE] = CONFIG_FILE_PATH_DEFAULT;

    if(argc>1){
        strncpy(config_path,argv[1],CADENA_SIZE);
    }

    cargar_variables_de_entorno_de_archivo(config_path);
    comprobar_variables_entorno();

    //Referido a limpieza al terminar
    configurar_at_exit_y_sigint();   
    g_err_msg = get_msg_to_transmit(TYPE_ERROR_FROM_SV,0, 0,NULL,&g_len_err_msg);

    //Aumentamos el numero de fd
    aumentar_limite_de_archivos_abiertos();

    //Referido a listas de clientes
    SLIST_INIT(&clientes_A_head);
    SLIST_INIT(&clientes_B_head);
    SLIST_INIT(&clientes_C_head);

    //Para iterar
    struct cliente* cp;

    //Referido a epoll
    int n_fds;
    struct epoll_event ep_eventos[MAX_EV];
    
    int efd = epoll_create1(0);
    if(efd == -1){
        perror("Error creating epoll");
        exit(1);
    }

    //Establecer conexiones
    int ipv4_port;
    int ipv6_port;
    sscanf(getenv(IPV4_PORT_ENV_NAME),"%d",&ipv4_port);
    sscanf(getenv(IPV6_PORT_ENV_NAME),"%d",&ipv6_port);
    
    listener_cliente_A_sfd = establecer_comunicacion_clientes_tipo_A(getenv(UNIX_PATH_ENV_NAME));
    listener_cliente_B_sfd = establecer_comunicacion_clientes_tipo_B(getenv(IPV4_IP_ENV_NAME),(uint16_t)ipv4_port);
    listener_cliente_C_sfd = establecer_comunicacion_clientes_tipo_C(getenv(IPV6_IP_ENV_NAME),(uint16_t)ipv6_port);

    //Agregamos a conexiones a epoll
    agregar_socket_a_epoll(efd,listener_cliente_A_sfd);
    agregar_socket_a_epoll(efd,listener_cliente_B_sfd);
    agregar_socket_a_epoll(efd,listener_cliente_C_sfd);

    //Declaramos variables para almacenar direcciones de clientes
    unsigned int cliente_A_len = sizeof(cliente_A_addr);
    unsigned int cliente_B_len = sizeof(cliente_B_addr);
    unsigned int cliente_C_len = sizeof(cliente_C_addr);
    
    char* recibido=NULL;

    //Entramos al ciclo while para procesar mensajes
    printf("Recibiendo mensajes:\n");
    while(1){
        /* Espero eventos */
        n_fds = epoll_wait(efd,ep_eventos,MAX_EV,-1);
        if(n_fds == -1){
            perror("Error esperando en epoll");
            exit(1);
        }
        /* Recorro los eventos */
        for(int i=0;i<n_fds;i++){
            /* En caso de ser el listener de cliente_A*/
            if(ep_eventos[i].data.fd == listener_cliente_A_sfd){
                aceptar_cliente_y_agregar_a_estructuras(listener_cliente_A_sfd, (struct sockaddr*) &cliente_A_addr,
                                                        &cliente_A_len, efd, &clientes_A_head);

                continue;
            }
            /* En caso de ser el listener de cliente_B*/
            if(ep_eventos[i].data.fd == listener_cliente_B_sfd){
                aceptar_cliente_y_agregar_a_estructuras(listener_cliente_B_sfd, (struct sockaddr*) &cliente_B_addr,
                                                        &cliente_B_len, efd, &clientes_B_head);

                continue;
            }
            /* En caso de ser el listener de cliente_C */
            if(ep_eventos[i].data.fd == listener_cliente_C_sfd){
                aceptar_cliente_y_agregar_a_estructuras(listener_cliente_C_sfd, (struct sockaddr*) &cliente_C_addr,
                                                        &cliente_C_len, efd, &clientes_C_head);

                continue;
            }        
            /* En caso de ser otro listener */

            /* En caso de ser de una conexion establecida de un cliente_A, leemos y procesamos el mensaje*/
            cp = SLIST_FIRST(&clientes_A_head);
            while(cp!=NULL){
                if(ep_eventos[i].data.fd == cp->sfd){
                    recibido = recibir_mensaje(ep_eventos[i].data.fd,efd);
                    if(recibido == NULL) break;

                    procesar_mensajes_tipo_A(recibido,ep_eventos[i].data.fd);
                    
                    free(recibido);
                    break;
                }
                cp = SLIST_NEXT(cp,clientes);

            }
            if(cp!=NULL) continue;

            /* En caso de ser de una conexion establecida de un cliente_B, leemos y procesamos el mensaje*/
            cp = SLIST_FIRST(&clientes_B_head);
            while(cp!=NULL){
                if(ep_eventos[i].data.fd == cp->sfd){
                    recibido = recibir_mensaje(ep_eventos[i].data.fd,efd);
                    if(recibido == NULL) break;
                    
                    procesar_mensajes_tipo_B(recibido,ep_eventos[i].data.fd);
                    
                    free(recibido);
                    break;
                }
                cp = SLIST_NEXT(cp,clientes);
            }
            if(cp!=NULL) continue;            
            
            cp = SLIST_FIRST(&clientes_C_head);
            while(cp!=NULL){
                if(ep_eventos[i].data.fd == cp->sfd){
                    recibido = recibir_mensaje(ep_eventos[i].data.fd,efd);
                    if(recibido == NULL) continue;

                    procesar_mensajes_tipo_C(recibido,ep_eventos[i].data.fd);

                    free(recibido);
                    break;    
                }
                cp = SLIST_NEXT(cp,clientes);
            }
                        
        }
        
    }

    return 0;
}

void configurar_at_exit_y_sigint(void){
    if(atexit(limpiar_comunicaciones)!=0){
        perror("Error seteando at_exit");
        exit(1);
    }

    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = sigint_handler;

    sigaction(SIGINT, &sa, NULL);
}

//////////////////////////////////////////////////////////////////////
///COMUNICACIONES GENERALES
void agregar_socket_a_epoll(int efd, int sfd){
    struct epoll_event ev;
    ev.data.fd = sfd;
    ev.events = EPOLLIN;

    if(epoll_ctl(efd,EPOLL_CTL_ADD,sfd,&ev)==-1){
        perror("Error agregando evento a epoll");
        exit(1);
    }
    return;

}

void agregar_cliente_a_lista(struct clientes_head* clientes_head_p, int sfd){
    struct cliente* cliente_p = malloc(sizeof(struct cliente));
    cliente_p->sfd = sfd;
    SLIST_INSERT_HEAD(clientes_head_p,cliente_p,clientes);
}

/**
 * Acepta el cliente dado un listener sfd, y lo agrega a las estructuras de epoll y 
 * lista de cliente correspondiente
*/
void aceptar_cliente_y_agregar_a_estructuras(int list_sfd, struct sockaddr* addr_p, unsigned int* addr_len_p, 
                                            int efd, struct clientes_head* ch_p){
    int new_sfd = accept(list_sfd,addr_p,addr_len_p);
    if(new_sfd == -1){
        perror("Error aceptando cliente");
        exit(1);
    }
    agregar_cliente_a_lista(ch_p,new_sfd);
    agregar_socket_a_epoll(efd,new_sfd);
}

void enviar_mensaje_a_lista_clientes(struct clientes_head* ch_p){
    struct cliente* cp  = NULL;
    
    cp = SLIST_FIRST(ch_p);
    while(cp!=NULL){
        send(cp->sfd,g_err_msg,g_len_err_msg,MSG_DONTWAIT);
        cp = SLIST_NEXT(cp,clientes);
    }

    return;
}

//Esto es más óptimo según el man
void eliminar_lista_clientes(struct clientes_head* clientes_head_p){
    struct cliente* cp;
    while (!SLIST_EMPTY(clientes_head_p)) {           /* List Deletion. */
                cp = SLIST_FIRST(clientes_head_p);
                close(cp->sfd);
                SLIST_REMOVE_HEAD(clientes_head_p, clientes);
                free(cp);
    }
    return;
}



void aumentar_limite_de_archivos_abiertos(void){
    struct rlimit new_limits;
    new_limits.rlim_cur = S_LIMIT_FOPEN;
    new_limits.rlim_max = H_LIMIT_FOPEN;

    const char* valgrind_value = getenv(VALGRIND_ENV_NAME);

    if(valgrind_value!=NULL){
        if(!strncmp(valgrind_value,"YES",strlen("YES"))){
            return;
        }
    }
    
    if(setrlimit(RLIMIT_NOFILE,&new_limits)==-1){
        perror("Error seteando limites de numeros de files abiertos");
        exit(1);
    }
}

/**
 * Recibe mensaje por sfd. En caso de desconectarse el cliente, elimina el sfd de epoll y lo cierra.
 * El mensaje retorna es alocado dinámicamente por lo que tiene que ser liberado usando free();
*/
char* recibir_mensaje(int sfd, int efd){
    size_t recibido_len=0;
    char* recibido = receive_data_msg(sfd,&recibido_len);
    if(recibido == NULL){
        switch(recibido_len){
            
            case 0: printf("El cliente se desconecto\n");
                    epoll_ctl(efd,EPOLL_CTL_DEL,sfd,NULL);
                    close(sfd);
                    break;
            
            case 1: printf("Error en el checksum\n");
                    break;
            
            default:break;
        }
    }
    
    return recibido;
}

////////////////////////////////////////////////////////////////////////////////////////////
///CLIENTE A

int establecer_comunicacion_clientes_tipo_A(const char* socket_path){
    return crear_y_bindear_unix_socket(socket_path);
}

int crear_y_bindear_unix_socket(const char* socket_path){
    int un_sfd;

    struct sockaddr_un my_addr;
    memset(&my_addr,0,sizeof(struct sockaddr_un));

    /*Socket creation*/
    un_sfd = socket(AF_UNIX,SOCK_STREAM,0);
    if(un_sfd==-1){
        perror("Error creating socket");
        exit(1);
    }
    int enable = 1;
    if (setsockopt(un_sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }
    
    my_addr.sun_family = AF_UNIX;
    strncpy(my_addr.sun_path,socket_path,sizeof(my_addr.sun_path)-1);

    unlink(socket_path);
    /*Bind*/
    if(bind(un_sfd,(struct sockaddr*)&my_addr,sizeof(my_addr))!=0){
        perror("Error while binding socket: ");
        exit(1);
    }

    /*Listen*/
    if(listen(un_sfd,MAX_EV)==-1){
        perror("Error while listening: ");
        exit(1);
    }

    return un_sfd;
}


void procesar_mensajes_tipo_A(char* mensaje, int sfd){
    char* resultado = NULL;
    char** requests = NULL;

    loguear_cliente_tipo_A(mensaje);

    requests = cJSON_get_requests(mensaje,NULL);
    resultado = get_output_journalctl_command(requests[0],MAX_SIZE_CL_A);
    if(resultado == NULL){
        resultado = calloc_safe(sizeof(char),CADENA_SIZE);
        strcpy(resultado,"Error: Comando inválido");
    }
    
    responder_mensaje_tipo_A(resultado, sfd);

    free(resultado);
    free_matrix(requests);
    return;
}

void responder_mensaje_tipo_A(char* response, int sfd){
    char* respuesta = armar_respuesta_tipo_A(response);
    enviar_respuesta_tipo_A(respuesta,sfd);
    
    free(respuesta);
}

char* armar_respuesta_tipo_A(char* response){
    char* mensaje = NULL;
    cJSON* responses = NULL;
    int n_responses = 0;

    cJSON* monitor = cJSON_get_header_response_by_server(&responses);

    // Responses
    cJSON_add_string_to_array(responses,"response_1",response);

    n_responses = cJSON_GetArraySize(responses);
    cJSON_replace_number_value(monitor,"n_responses",n_responses);

    mensaje = cJSON_Print(monitor);
    if(mensaje==NULL){
        perror("Error imprimiendo json");
        exit(1);
    }

    cJSON_Delete(monitor);

    return mensaje;
}

void enviar_respuesta_tipo_A(char* respuesta, int sfd){
    ssize_t bytes_sended;
    bytes_sended = send_data_msg(sfd,respuesta,strlen(respuesta),0);
    if(bytes_sended==-1){
        perror("Error enviando mensaje tipo A");
        exit(1);
    }
}

void loguear_cliente_tipo_A(char* cadena){
    printf("Recibo: %s\n",cadena);
    loguear_mensaje_cliente('A',cadena);
    n_mensajes_tipo_A++;
    imprimir_cantidad_de_mensajes_recibidos();
}

//////////////////////////////////////////////////////////////////////
///CLIENTE B

int establecer_comunicacion_clientes_tipo_B(const char* ip, uint16_t port){
    return crear_y_bindear_inet_socket(ip,port);
}

int crear_y_bindear_inet_socket(const char* ip, uint16_t port){
    int in_sfd;

    struct sockaddr_in my_addr;
    memset(&my_addr,0,sizeof(struct sockaddr_in));

    /*Socket creation*/
    in_sfd = socket(AF_INET,SOCK_STREAM,0);
    if(in_sfd==-1){
        perror("Error creating socket");
        exit(1);
    }
    int enable = 1;
    if (setsockopt(in_sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }
    
    memset(&my_addr,0,sizeof(struct sockaddr_in));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = inet_addr(ip);
    my_addr.sin_port = htons(port);

    /*Bind*/
    if(bind(in_sfd,(struct sockaddr*)&my_addr,sizeof(my_addr))!=0){
        perror("Error while binding socket: ");
        exit(1);
    }

    /*Listen*/
    if(listen(in_sfd,MAX_EV)==-1){
        perror("Error while listening: ");
        exit(1);
    }

    return in_sfd;
}

void procesar_mensajes_tipo_B(char* mensaje, int sfd){
    char* resultado = NULL;
    char** requests = NULL;

    loguear_cliente_tipo_B(mensaje);

    requests = cJSON_get_requests(mensaje,NULL);
    resultado = get_output_journalctl_command(requests[0],MAX_SIZE_CL_B);
    if(resultado == NULL){
        send_data_msg(sfd,"Error: Comando invalido",strlen("Error: Comando invalido"),MSG_DONTWAIT);
        free_matrix(requests);
        return;    
    }
    
    responder_mensaje_tipo_B(sfd,resultado);

    free(resultado);
    free_matrix(requests);

    return;
}

void comprimir_resultado(char* a_comprimir){
    compress_one_file_from_var(a_comprimir, (unsigned int) strlen(a_comprimir), TEMPORARY_COMPRESS_PATH);
}

void responder_mensaje_tipo_B(int sfd, char* resultado){
    comprimir_resultado(resultado);
    enviar_comprimido(sfd);
    if(remove(TEMPORARY_COMPRESS_PATH)==-1){
        perror("Error while removing compressed log: ");
        exit(1);
    }
}

void enviar_comprimido(int sfd){
    FILE* fp = fopen(TEMPORARY_COMPRESS_PATH,"rb");
    size_t num_read = 0;
    char buffer[MSG_DATA_SIZE];
    unsigned int count = 0;
    size_t len;

    while((num_read = fread(buffer,1,sizeof(buffer),fp))>0){
        char* to_send = get_msg_to_transmit(1,count++, num_read & 0x7fffffff,buffer,&len);
        if(send(sfd,to_send,len,0) == -1){
            perror("Error while sending log");
            exit(1);
        }
        free(to_send);
    }

    fclose(fp);
    return;
}

void loguear_cliente_tipo_B(char* cadena){
    printf("Recibo: %s\n",cadena);
    loguear_mensaje_cliente('B',cadena);
    n_mensajes_tipo_B++;
    imprimir_cantidad_de_mensajes_recibidos();

}

//////////////////////////////////////////////////////////////////////
///CLIENTE C

int establecer_comunicacion_clientes_tipo_C(const char* ip, uint16_t port){
    return crear_y_bindear_inet6_socket(ip,port);
}

int crear_y_bindear_inet6_socket(const char* ip, uint16_t port){
    int in6_sfd;

    struct sockaddr_in6 my_addr;
    memset(&my_addr,0,sizeof(struct sockaddr_in6));

    /*Socket creation*/
    in6_sfd = socket(AF_INET6,SOCK_STREAM,0);
    if(in6_sfd==-1){
        perror("Error creating socket");
        exit(1);
    }
    int enable = 1;
    if (setsockopt(in6_sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }
    
    memset(&my_addr,0,sizeof(struct sockaddr_in6));
    my_addr.sin6_family = AF_INET6;
    my_addr.sin6_port = htons(port);
    if(inet_pton(AF_INET6,ip,&my_addr.sin6_addr)==-1){
        perror("Error convirtiendo direccion ipv6");
        exit(1);
    }

    /*Bind*/
    if(bind(in6_sfd,(struct sockaddr*)&my_addr,sizeof(my_addr))!=0){
        perror("Error while binding socket: ");
        exit(1);
    }

    /*Listen*/
    if(listen(in6_sfd,MAX_EV)==-1){
        perror("Error while listening: ");
        exit(1);
    }

    return in6_sfd;
}

void procesar_mensajes_tipo_C(char* mensaje, int sfd){
    
    loguear_cliente_tipo_C(mensaje);  
    responder_mensajes_tipo_C(sfd);
    
    return;
}

void responder_mensajes_tipo_C(int sfd){
    float carga_normalizada;
    int memoria_libre;
    char* respuesta;

    carga_normalizada = obtener_carga_normalizada();
    memoria_libre = obtener_memoria_libre();

    respuesta = armar_respuesta_tipo_C(carga_normalizada,memoria_libre);
    enviar_respuesta_tipo_C(respuesta,sfd);
    
    free(respuesta);
    return;
}

float obtener_carga_normalizada(void){
    float carga = obtener_carga();
    int n_cores = obtener_cpu_cores();
    return carga/(float)n_cores;
}
#define BUFFER_PROC_SIZE 2048

int obtener_cpu_cores(void){
    char buffer[BUFFER_PROC_SIZE], *match;
    int n_cores;
    
    leer_archivo("/proc/cpuinfo",buffer);
    match = strstr(buffer, "cpu cores");
    sscanf(match,"cpu cores : %d",&n_cores);
    
    return n_cores;
}

float obtener_carga(void){
    char buffer[BUFFER_PROC_SIZE];
    float carga, aux1, aux2;
    
    leer_archivo("/proc/loadavg",buffer);
    sscanf(buffer,"%f %f %f",&aux1,&aux2,&carga);
    
    return carga;
}

int obtener_memoria_libre(void){
    char buffer[BUFFER_PROC_SIZE], *match;
    int mem_libre;

    leer_archivo("/proc/meminfo",buffer);
    match = strstr(buffer, "MemFree:");
    sscanf(match, "MemFree: %d",&mem_libre);

    return mem_libre;
}

void leer_archivo(char* path, char* buffer){
    FILE* fp;
    size_t bytes_read;

    fp = fopen(path,"r");
    if(fp==NULL){
        perror("Error leyendo archivo");
        exit(1);
    }
    bytes_read = fread(buffer,1,BUFFER_PROC_SIZE,fp);
    fclose(fp); 
    
    if(bytes_read == 0){
        perror("Archivo vacio");
        exit(1);
    }

    buffer[bytes_read] = '\0';

}

/**
 * Forma:
 * {
 *      "origen":   "servidor",
 *      "n_responses": ?,
 *      "responses":  [
 *              { "mem_free"        : num(mem_free)        },
 *              { "norm_load_avg"   : num(norm_load_avg)   }
 *      ] 
 * }
*/
char* armar_respuesta_tipo_C(float carga_normalizada, int memoria_libre){
    char* mensaje = NULL;
    cJSON* responses = NULL;
    int n_responses = 0;

    cJSON* monitor = cJSON_get_header_response_by_server(&responses);

    // Responses
    cJSON_add_number_to_array(responses,"mem_free"      ,(double)memoria_libre);
    cJSON_add_number_to_array(responses,"norm_load_avg" ,(double)carga_normalizada);

    n_responses = cJSON_GetArraySize(responses);
    cJSON_replace_number_value(monitor,"n_responses",(double)n_responses);

    mensaje = cJSON_Print(monitor);
    if(mensaje==NULL){
        perror("Error imprimiendo json");
        exit(1);
    }

    cJSON_Delete(monitor);

    return mensaje;
    
}

void enviar_respuesta_tipo_C(char* mensaje, int sfd){
    ssize_t bytes_sended;

    bytes_sended = send_data_msg(sfd,mensaje,strlen(mensaje),MSG_DONTWAIT);
    if(bytes_sended == 0){
        printf("El cliente se desconecto\n");
    }
}

void loguear_cliente_tipo_C(char* cadena){
    printf("Recibo: %s\n",cadena);
    loguear_mensaje_cliente('C',cadena);
    n_mensajes_tipo_C++;
    imprimir_cantidad_de_mensajes_recibidos();

}


//////////////////////////////////////////////////////////////////////
///LOGS

/**
 * Retorna un puntero dinámicamente alocado con la fecha en formato año-mes-dia hora:min:seg
 * Extraido y ligeramente modificado de 
 * https://stackoverflow.com/questions/1442116/how-to-get-the-date-and-time-values-in-a-c-program
 * 
*/
char* obtener_fecha(void){
    char* fecha = calloc(sizeof(char),FECHA_SIZE);
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(fecha,"%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    
    return fecha;
}

/**
 * Loguea mensaje con formato
 * 'datetime' A -> mensaje
 * */
void loguear_mensaje_cliente(char tipo,char* mensaje){
    char* fecha = NULL;
    char a_loguear[2*CADENA_SIZE];

    fecha = obtener_fecha();

    memset(a_loguear,0,2*CADENA_SIZE);
    sprintf(a_loguear,"%s\t\t%c\t -> \t%s\n",fecha,tipo,mensaje);

    loguear(a_loguear);

    free(fecha);
}

void loguear(char* cadena){
    FILE* fp;
    fp = fopen(getenv(LOG_PATH_ENV_NAME),"a");
    if(fp==NULL){
        perror("Error abriendo log");
        exit(1);
    }

    fprintf(fp,"%s",cadena);

    if(fclose(fp)!=0){
        perror("Error cerrando log");
        exit(1);
    }
}

void imprimir_cantidad_de_mensajes_recibidos(void){
    printf("\t\t\t\t\tA\tB\tC\n");
    printf("Mensajes recibidos: \t\t\t%d\t%d\t%d\n",n_mensajes_tipo_A,n_mensajes_tipo_B,n_mensajes_tipo_C);
    printf("Total: %d\n",n_mensajes_tipo_A+n_mensajes_tipo_B+n_mensajes_tipo_C);
}


//////////////////////////////////////////////////////////////////////
///FILE

void crear_archivo_si_no_existe(const char* path){
    FILE* fp;

    fp = fopen(path,"a");
    if(fp==NULL){
        perror("Error abriendo archivo");
        fprintf(stderr,"%s\n",path);
        exit(1);
    }
    if(fclose(fp)==-1){
        perror("Error cerrando archivo");
        exit(1);
    }
    return;
}

/////////////////////////////////////////////////////////////////////
///OBTENER SALIDA DE JOURNALCTL

/**
 * Executes a journalctl command and returns a string with the output. 
 * The output is dinamically alocated, so it must be freed
 * Return 'max size' bytes as maximum
*/
char* get_output_journalctl_command(char* journalctl_command,size_t max_size){
//    char* comm_arg[64] = {"journalctl", "-u", "ondemand.service",NULL};
    char** comm_arg = NULL;
    int pipe_fds [2];   //pipe_fds[0] -> read-end ; [1] -> write end
    char buff[4096];
    ssize_t bytes_read;
    char* cadena_completa=NULL;
    size_t len_cadena_completa=0;
    pid_t pid;

    comm_arg = split_args(journalctl_command,NULL);
    if(strncmp(comm_arg[0],"journalctl",strlen("journalctl"))|| strlen(comm_arg[0])>strlen("journalctl")){
        printf("Error, comando invalido\n");
        free_matrix(comm_arg);
        return NULL;
    }

    if(pipe(pipe_fds)==-1){
        perror("Error creando pipe");
        exit(1);
    }

    pid = fork();
    switch(pid){
        case -1:
            perror("Error forkeando");
            exit(1);
        
        case 0:
            //Child code
            set_pipe_as_stdout(pipe_fds);
            if((execvp(comm_arg[0],comm_arg))==-1){
                perror("Error execvp");
                exit(1);
            }
            break;
        default:
            //Parent code
            close(pipe_fds[1]);
            memset(buff,0,4096);
            while((bytes_read=read(pipe_fds[0],buff,4095))!=0){
                if(bytes_read==-1){
                    perror("Error");
                    exit(1);
                }
                cadena_completa = realloc_safe(cadena_completa,(size_t)bytes_read+len_cadena_completa+1);

                strncpy(cadena_completa+len_cadena_completa,buff,(size_t)bytes_read);
                len_cadena_completa += (size_t) bytes_read;
                cadena_completa[len_cadena_completa] = '\0';
                
                if(len_cadena_completa>max_size){
                    free_matrix(comm_arg);
                    free(cadena_completa);
                    return NULL;
                }

                memset(buff,0,4096);

            }
            esperar_hijo(pid);
    }

    free_matrix(comm_arg);
    return cadena_completa;
}

/**
 * Espera bloqueante a que el proceso hijo termine su ejecucion
*/
void esperar_hijo(pid_t pid){
    int status;
    if(waitpid(pid,&status,0)==-1){
        perror("Error esperando");
        exit(1);
    }
}

/**
 * Closes the read-end pipe fd, and set the write-end pipe fd as the stdout of the
 * calling process using dup2; 
*/
void set_pipe_as_stdout(int pipe_fds[2]){
    if(close(pipe_fds[0])==-1){
        perror("Error cerrando fd de pipe");
        exit(1);
    }
    if(dup2(pipe_fds[1],STDOUT_FILENO)==-1){
        perror("Error seteando el fd como stdout");
        exit(1);
    }
    if(close(pipe_fds[1])==-1){
        perror("Error cerrando fd de pipe");
        exit(1);
    }
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

/**
 * Realloc method with error control
*/
void* realloc_safe(void* ptr, size_t size){
    void* aux = realloc(ptr,size);
    if(aux == NULL){
        perror("Error while reallocating");
        exit(1);
    }
    return aux;
}

/**
 * Calloc method with error control
*/
void* calloc_safe(size_t __nmemb, size_t __size){
    void* aux = calloc(__nmemb,__size);
    if(aux == NULL){
        perror("Error while callocing");
        exit(1);
    } 
    return aux;
}

/**
 * Returns a pointer to an dynamically allocated 2 dimensional array that contains the argumments splited.
 * E.g. 
 *    If command content is:
 *                      ping google.com.ar -c 6
 * 
 *  Then, args[0] = "ping", commands[1] = "google.com.ar", commands[2] = "-c" and commands[3] = "6"
 *   This function uses strtok, but does not modifies command
*/
char** split_args(char* command, unsigned int* n_args_p){
    return split_words_safely(command, n_args_p, " \t");
}


/**
 * Returns a pointer to an dynamically allocated 2 dimensional array that contains strings separates 
 * by any of the characters in pattern.
 * The number of items splitted is stored in n_items_p
 * This function uses strtok, but does not modifies buffer
*/
char** split_words_safely(char* buffer, unsigned int* n_items_p, char* pattern){
    char** items=NULL;
    char* aux=NULL;
    unsigned int counter = 0;

    //Copy for safe work
    char* buffer_safe = calloc_safe(strlen(buffer)+1,sizeof(char));
    strncpy(buffer_safe,buffer,strlen(buffer));

    aux = strtok(buffer_safe,pattern);

    while(aux!=NULL){
        items = realloc_safe(items,(counter+1) * sizeof(char*));
        items[counter] = calloc_safe(strlen(aux)+1,sizeof(char));
        strncpy(items[counter],aux,strlen(aux));

        counter++;
        aux = strtok(NULL,pattern);
    }
    items = realloc_safe(items,(counter+1) * sizeof(char*));
    items[counter] = NULL;

    if(n_items_p != NULL){
        *n_items_p = counter;
    }

    free(buffer_safe);
    return items;
}

