#define CONFIG_FILE_PATH_DEFAULT                "../config/ipc.conf"

/*ENV NAMES PATHS*/
#define LOG_PATH_ENV_NAME                                 "LOG_PATH"
#define COMPRESS_PATH_ENV_NAME                       "COMPRESS_PATH"

/*ENV NAMES UNIX*/
#define UNIX_PATH_ENV_NAME                               "UNIX_PATH"


/*ENV NAMES IPv4*/
#define IPV4_IP_ENV_NAME                                   "IPV4_IP"
#define IPV4_PORT_ENV_NAME                               "IPV4_PORT"

/*ENV NAMES IPv6*/
#define IPV6_IP_ENV_NAME                                   "IPV6_IP"
#define IPV6_PORT_ENV_NAME                               "IPV6_PORT"

/*DEFAULT PATHS*/
#define LOG_PATH_DEFAULT                       "../log/mensajes.log"
#define COMPRESS_PATH_DEFAULT                 "../compressed_files/"

/*DEFAULT UNIX*/
#define UNIX_PATH_DEFAULT         "/tmp/ffalkjdflkjasdnflkjasndflas"

/*DEFAULT IPv4*/
#define IPV4_IP_DEFAULT                                 "127.0.0.1"
#define IPV4_PORT_DEFAULT                                    "5050"

/*DEFAULT IPv6*/
#define IPV6_IP_DEFAULT                                       "::1" //0:0:0:0:0:0:1
#define IPV6_PORT_DEFAULT                                    "5060"


/*FUNCTIONS DEFINITION*/
void comprobar_variables_entorno                        ();
void comprobar_variables_entorno_A                      ();
void comprobar_variables_entorno_B                      ();
void comprobar_variables_entorno_C                      ();
void comprobar_variables_entorno_log                    ();
void cargar_variables_de_entorno_de_archivo             (const char* path);