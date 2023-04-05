// Esto es solamente para que la ide no moleste con no encontrar la estructura de sigaction
#define _XOPEN_SOURCE 700

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
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/epoll.h>
#include "checksum.h"

#define CADENA_SIZE                                 64
#define FECHA_SIZE                                  20

void    configurar_at_exit_y_sigint                 ();
void    crear_archivo_si_no_existe                  (const char* path);

//////////////////////////////////////////////////////////////////////////////
/// Referido a comunicacion en general


struct listener{
    int                           sfd;
    addr_union                  *addr;
    SLIST_ENTRY(listener)   listeners;
};
SLIST_HEAD(slist_head,listener);


typedef union{
    struct sockaddr_un*      addr_un;
    struct sockaddr_in*      addr_in;
    struct sockaddr_in6*     addr_in6;
}addr_union;

#define MAX_EV                          5000
#define S_LIMIT_FOPEN                   5050
#define H_LIMIT_FOPEN                   8000

#define N_BYTES_TO_RECEIVE               256

struct listener*    crear_y_bindear_inet_socket             (char* ip_dir);//,uint16_t port);
struct listener*    crear_y_bindear_unix_socket             (const char* socket_path);
void                agregar_socket_a_epoll                  (int efd, int sfd);
void                aumentar_limite_de_archivos_abiertos    ();

//////////////////////////////////////////////////////////////////////////////
/// Referido a cliente A
#define UNIX_SOCKET_PATH                          "/tmp/ffalkjdflkjasdnflkjasndflas"
//#define SOCKET_PATH_A_ENV_NAME

int     establecer_comunicacion_clientes_tipo_A     (const char* path_fifo);
void    procesar_mensajes_tipo_A                    (char* mensaje, int fd);
ssize_t leer_clientes_tipo_A                        (char* cadena, int fd);
void    loguear_cliente_tipo_A                      (char* cadena);
void    imprimir_cantidad_de_mensajes_recibidos     ();

//////////////////////////////////////////////////////////////////////////////
/// Referido a cliente B

int     establecer_comunicacion_clientes_tipo_B     (const char* msgq_path);
void    procesar_mensajes_tipo_B                    (int msgq_id);
ssize_t leer_clientes_tipo_B                        (char* cadena, int msgq_id);
void    loguear_cliente_tipo_B                      (char* cadena);

//////////////////////////////////////////////////////////////////////////////
/// Referido a cliente C

char    establecer_comunicacion_clientes_tipo_C     (const char* shm_path);
void    procesar_mensajes_tipo_C                    ();
ssize_t leer_clientes_tipo_C                        ();
void    loguear_cliente_tipo_C                      (char* cadena);


//////////////////////////////////////////////////////////////////////////////
/// Referido a logs

void    loguear                                     (char* cadena);
void    loguear_mensaje_cliente                     (char tipo,char* mensaje);
char*   obtener_fecha                               ();


/**
 * At exit
*/
void limpiar_comunicaciones(void){

}

/**
 * SIGINT HANDLER
*/
void sigint_handler(int num){
    num++;  //Por el warning de no usar num
    exit(1);
}

/**
 * VARIABLES GLOBALES
*/

unsigned int n_mensajes_tipo_A;
unsigned int n_mensajes_tipo_B;
unsigned int n_mensajes_tipo_C;

//////////////////////////////////////MAIN///////////////////////////////////////

int main(int argc, char* argv[]){
    struct listener* listener_cliente_A = NULL;
    struct sockaddr_un cliente_A_addr;
    char msg[N_BYTES_TO_RECEIVE];
    ssize_t bytes_read;

    //Referido a carga de variables de entorno
    char config_path[CADENA_SIZE] = CONFIG_FILE_PATH_DEFAULT;

    if(argc>1){
        strncpy(config_path,argv[1],CADENA_SIZE);
    }

    cargar_variables_de_entorno_de_archivo(config_path);
    comprobar_variables_entorno();

    //Referido a limpieza al terminar
    configurar_at_exit_y_sigint();   

    //Referido a epoll
    int n_fds;
    struct epoll_event ep_eventos[MAX_EV];
    
    int efd = epoll_create1(0);
    if(efd == -1){
        perror("Error creating epoll");
        exit(1);
    }

    //Establecer conexiones
    listener_cliente_A = establecer_comunicacion_clientes_tipo_A(UNIX_SOCKET_PATH);

    //Agregamos a conexiones a epoll
    agregar_socket_a_epoll(efd,listener_cliente_A->sfd);

    //Declaramos variables para almacenar direcciones de clientes
    unsigned int cliente_A_len = sizeof(cliente_A_addr);
    int new_sfd;
    
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
            if(ep_eventos[i].data.fd == listener_cliente_A->sfd){
                new_sfd = accept(listener_cliente_A->sfd,&cliente_A_addr,&cliente_A_len);
                if(new_sfd == -1){
                    perror("Error aceptando cliente_A");
                    exit(1);
                }
                //TODO Considerar agregarlo a una lista para posteriormente limpiar estructuras y/o enviar mensaje de desconexion
                agregar_socket_a_epoll(efd,new_sfd);
                continue;
            }

            /* En caso de ser otro listener */

            /* En caso de ser de una conexion establecida, leemos y procesamos el mensaje*/
            memset(msg,0,N_BYTES_TO_RECEIVE);
            bytes_read = receive_msg(ep_eventos[i].data.fd,msg);
            if(bytes_read == 0){
                printf("El cliente se desconecto\n");
                epoll_ctl(efd,EPOLL_CTL_DEL,ep_eventos[i].data.fd,NULL);
                close(ep_eventos[i].data.fd);
                break;
            }
            /* 
            printf("Recibo: \n");
            for(ssize_t i=0; i<bytes_read; i++){
                printf("%02hhx",msg[i]);
            }
            printf("\n"); */

            process_msg_A(msg,(size_t)bytes_read,ep_eventos[i].data.fd);
        }

        //procesar_mensajes_tipo_A();
        //procesar_mensajes_tipo_B(g_msgq_id);
        //procesar_mensajes_tipo_C(shm_p);
    }

    return 0;
}

void configurar_at_exit_y_sigint(){
    if(atexit(limpiar_comunicaciones)!=0){
        perror("Error seteando at_exit");
        exit(1);
    }

    struct sigaction sa;
    memset(&sa,0,sizeof(sa));           //Para no tener errores de valgrind
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

void aumentar_limite_de_archivos_abiertos(){
    struct rlimit new_limits;
    new_limits.rlim_cur = S_LIMIT_FOPEN;
    new_limits.rlim_max = H_LIMIT_FOPEN;
    if(setrlimit(RLIMIT_NOFILE,&new_limits)==-1){
        perror("Error seteando limites de numeros de files abiertos");
        exit(1);
    }
}

void process_msg_A(char* msg, size_t len, int sfd){
    //printf("%s\n",msg);
    msg_struct_t* msg_struct = get_msg_struct_from_msg_received(msg);

    if(!is_checksum_ok(msg,len,msg_struct->md_value)){
        //Do something (or not) to take care off
        return;
    }

    procesar_mensajes_tipo_A(msg_struct->data, sfd);
    
    free(msg_struct->data);
    free(msg_struct);
}


//////////////////////////////////////////////////////////////////////
///CLIENTE A

int establecer_comunicacion_clientes_tipo_A(const char* socket_path){
    return crear_y_bindear_unix_socket(socket_path);
}

struct listener* crear_y_bindear_unix_socket(const char* socket_path){
    struct listener* listener_struct = malloc(sizeof(struct listener));
    struct sockaddr_un* my_addr = calloc(1,sizeof(struct sockaddr_un));
    if(my_addr==NULL || listener_struct == NULL){
        perror("Error en calloc o malloc creando/bindeando sockets");
        exit(1);
    }

    /*Socket creation*/
    listener_struct->sfd = socket(AF_UNIX,SOCK_STREAM,0);
    if(listener_struct->sfd==-1){
        perror("Error creating socket");
        exit(1);
    }
    int enable = 1;
    if (setsockopt(listener_struct->sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }
    
    my_addr->sun_family = AF_UNIX;
    strncpy(my_addr->sun_path,socket_path,sizeof(my_addr->sun_path)-1);

    /*Bind*/
    if(bind(listener_struct->sfd,(struct sockaddr*)my_addr,sizeof(*my_addr))!=0){
        perror("Error while binding socket: ");
        exit(1);
    }

    /*Listen*/
    if(listen(listener_struct->sfd,1)==-1){
        perror("Error while listening: ");
        exit(1);
    }

    listener_struct->addr = my_addr;

    return listener_struct;
}


void procesar_mensajes_tipo_A(char* mensaje, int sfd){
    printf("%s\n",mensaje);
    loguear_cliente_tipo_A(mensaje);

}

void loguear_cliente_tipo_A(char* cadena){
    printf("Recibo: %s\n",cadena);
    loguear_mensaje_cliente('A',cadena);
    n_mensajes_tipo_A++;
    imprimir_cantidad_de_mensajes_recibidos();
}

//////////////////////////////////////////////////////////////////////
///CLIENTE B

int establecer_comunicacion_clientes_tipo_B(const char* msgq_path){
    key_t key;
    int   msgq_id;

    crear_archivo_si_no_existe(msgq_path);
    
    key = ftok(msgq_path,1);    
    if(key==-1){
        perror("Error obteniendo key en servidor");
        exit(1);
    }
    
    msgq_id = msgget(key,IPC_CREAT | 0660);
    if(msgq_id==-1){
        perror("Error obteniendo la msgq en servidor");
        exit(1);
    }

    return msgq_id;
}


void procesar_mensajes_tipo_B(int msgq_id){
    char cadena[CADENA_SIZE];
    memset(cadena,0,CADENA_SIZE);

    if(leer_clientes_tipo_B(cadena,msgq_id)<=0){
        return;
    }

    loguear_cliente_tipo_B(cadena);
    imprimir_cantidad_de_mensajes_recibidos();

}

ssize_t leer_clientes_tipo_B(char* cadena, int msgq_id){
    ssize_t bytes_read=0;

    return bytes_read;
}

void loguear_cliente_tipo_B(char* cadena){
    printf("Recibo: %s\n",cadena);
    loguear_mensaje_cliente('B',cadena);
    n_mensajes_tipo_B++;
}

//////////////////////////////////////////////////////////////////////
///CLIENTE C

char establecer_comunicacion_clientes_tipo_C(const char* shm_path){

    return 'a';
}


void procesar_mensajes_tipo_C(){
    char cadena[CADENA_SIZE];
    memset(cadena,0,CADENA_SIZE);

    if(leer_clientes_tipo_C()<=0){
        return;
    }

    loguear_cliente_tipo_C(cadena);
    imprimir_cantidad_de_mensajes_recibidos();

}

ssize_t leer_clientes_tipo_C(){
    ssize_t bytes_read = 0;
    
    return bytes_read;
}

void loguear_cliente_tipo_C(char* cadena){
    printf("Recibo: %s\n",cadena);
    loguear_mensaje_cliente('C',cadena);
    n_mensajes_tipo_C++;
}


//////////////////////////////////////////////////////////////////////
///LOGS

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

void imprimir_cantidad_de_mensajes_recibidos(){
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